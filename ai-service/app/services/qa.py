"""RAG 问答编排：向量宽召回 → 交叉编码器精排 → LLM 总结 / 抽取式兜底。

检索增强两段式：向量库先宽召回 ``retrieve_k`` 条候选（双塔，快但粗），再用 reranker
交叉编码器精排取前几条（精度高）喂给 LLM。精排不可用（禁用/加载失败）时自动退回纯向量分
排序——见 ``app.services.reranker``。

无 LLM 密钥（或端点抖动）必通：``llm.is_configured()`` 为假，或 ``llm.chat`` 抛错/空答，
都落到抽取式兜底（返回高分原文片段 + 引用），保证无密钥也能端到端跑通。

本服务不持有 ``kb_article.title``（向量块 metadata 仅 article_id/seq/status），
citations.title 暂用占位「知识 #{id}」，真实标题由 Java 编排层（M9.3）回填。
权限与上线状态过滤由上游 Java 传 ``allowed_article_ids`` 完成。
"""
from __future__ import annotations

import re

from app.core.config import settings
from app.core.logging import logger
from app.services import embedding, llm, reranker
from app.services.vector_store import get_vector_store

_NO_HIT_MSG = "未找到相关内容，建议转人工客服。"
_SNIPPET_MAX = 120  # 引用片段展示截断长度（字符）

_SYSTEM_PROMPT = (
    "你是知识库智能客服助手。只能依据下面【参考资料】回答用户问题，"
    "不得编造资料之外的信息；若资料不足以回答，就明确说明没有找到相关内容并建议转人工客服。"
    "请用简体中文简洁作答。"
)


def _eff(h: dict) -> float:
    """有效相关分：有精排分用精排分（更可信），否则回落向量余弦分。

    精排开启时每个命中都带 ``rerank_score``；不可用时只有 ``score``——以此统一排序/阈值/展示口径。
    """
    return h.get("rerank_score", h["score"])


def _compress(text: str) -> str:
    """压空白：连续空白（含换行）折叠为单空格，便于片段展示与喂给 LLM。"""
    return re.sub(r"\s+", " ", text or "").strip()


def _snippet(text: str, limit: int = _SNIPPET_MAX) -> str:
    s = _compress(text)
    return s if len(s) <= limit else s[:limit] + "…"


def _best_per_article(hits: list[dict]) -> list[dict]:
    """按 article_id 去重，同篇取最高分块；结果按分数降序（最相关在前）。仅用于引用展示。"""
    best: dict[int, dict] = {}
    for h in hits:
        aid = h["article_id"]
        if aid not in best or _eff(h) > _eff(best[aid]):
            best[aid] = h
    return sorted(best.values(), key=_eff, reverse=True)


def _select_context(ranked: list[dict], limit: int, max_per_article: int) -> list[dict]:
    """精排后选送 LLM 的上下文：按相关性序取最多 limit 块，单篇最多 max_per_article 块。

    允许同一篇出多块——答案常跨相邻块，旧版「每篇只留 1 块」会把单篇深答案截没；同时限制
    单篇占比，避免一篇刷屏挤掉其他相关知识。入参 ranked 须已按 _eff 降序。
    """
    out: list[dict] = []
    per: dict[int, int] = {}
    for h in ranked:
        aid = h["article_id"]
        if per.get(aid, 0) >= max_per_article:
            continue
        out.append(h)
        per[aid] = per.get(aid, 0) + 1
        if len(out) >= limit:
            break
    return out


def _no_hit() -> dict:
    return {"answer": _NO_HIT_MSG, "citations": [], "mode": "no_hit"}


def _build_citations(refs: list[dict]) -> list[dict]:
    return [
        {
            "article_id": r["article_id"],
            "title": f"知识 #{r['article_id']}",  # 占位，M9.3 由 Java 回填真实标题
            "score": round(_eff(r), 4),
            "snippet": _snippet(r["text"]),
        }
        for r in refs
    ]


def _reference_block(refs: list[dict]) -> str:
    """把召回片段拼成喂给 LLM 的【参考资料】文本。"""
    return "\n".join(f"[资料{i}] {_compress(r['text'])}" for i, r in enumerate(refs, 1))


def _extractive_answer(refs: list[dict]) -> str:
    """抽取式兜底：直接把高分原文片段拼成答案（无 LLM 时用）。"""
    parts = "\n".join(f"{i}. {_compress(r['text'])}" for i, r in enumerate(refs, 1))
    return "根据知识库，找到以下相关内容：\n" + parts


def answer(
    question: str,
    knowledge_type: str = "SCRIPT",
    top_k: int = 5,
    allowed_article_ids: list[int] | None = None,
) -> dict:
    """检索增强问答，返回 {answer, citations, mode}。

    流程：embed → 向量宽召回 retrieve_k 条 → 精排 → 相关性下限过滤 → 选上下文（同篇多块、
    单篇封顶）→ LLM 总结 / 抽取式兜底。``top_k`` 为最终送 LLM / 引用展示的条数上限。

    mode：``llm``（LLM 总结）/ ``extractive``（抽取式兜底）/ ``no_hit``（空问题或无相关知识）。
    """
    q = (question or "").strip()
    if not q:
        return _no_hit()

    qvec, _ = embedding.embed_query(q)
    # 宽召回：候选池至少覆盖请求 top_k，再多取些给精排挑（召回全、精排准）
    pool_k = max(settings.retrieve_k, top_k)
    pool = get_vector_store(knowledge_type).query(qvec, top_k=pool_k, allowed_article_ids=allowed_article_ids)
    if not pool:
        return _no_hit()

    # 精排：开启则交叉编码器打分（写回 rerank_score）；不可用时原样按向量分降序、不标注
    ranked = (
        reranker.rerank(q, pool)
        if reranker.is_enabled()
        else sorted(pool, key=lambda h: h["score"], reverse=True)
    )

    # 相关性下限：精排生效用 rerank_min_score，否则用 qa_min_score；_eff 保证两条路口径自洽
    reranked = bool(ranked) and "rerank_score" in ranked[0]
    min_score = settings.rerank_min_score if reranked else settings.qa_min_score
    ranked = [h for h in ranked if _eff(h) >= min_score]
    if not ranked:
        return _no_hit()

    # 送 LLM 的上下文：同篇可多块（封顶 max_per_article），总数封顶 top_k
    refs = _select_context(ranked, top_k, settings.max_per_article)
    # 引用展示：与上下文同源、按篇去重（同篇取最相关块），忠实反映「答案依据」
    citations = _build_citations(_best_per_article(refs))

    # 优先 LLM 总结；未配置或调用失败/空答都落到抽取式兜底
    if llm.is_configured():
        try:
            user_prompt = f"【参考资料】\n{_reference_block(refs)}\n\n【问题】{q}"
            text = llm.chat(_SYSTEM_PROMPT, user_prompt)
            if text:
                return {"answer": text, "citations": citations, "mode": "llm"}
            logger.warning("LLM 返回空答案，落抽取式兜底")
        except Exception:
            logger.exception("LLM 调用失败，落抽取式兜底")

    return {"answer": _extractive_answer(refs), "citations": citations, "mode": "extractive"}
