"""RAG 问答编排：向量检索召回 → LLM 总结 / 抽取式兜底。

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
from app.services import embedding, llm
from app.services.vector_store import get_vector_store

_NO_HIT_MSG = "未找到相关内容，建议转人工客服。"
_SNIPPET_MAX = 120  # 引用片段展示截断长度（字符）

_SYSTEM_PROMPT = (
    "你是知识库智能客服助手。只能依据下面【参考资料】回答用户问题，"
    "不得编造资料之外的信息；若资料不足以回答，就明确说明没有找到相关内容并建议转人工客服。"
    "请用简体中文简洁作答。"
)


def _compress(text: str) -> str:
    """压空白：连续空白（含换行）折叠为单空格，便于片段展示与喂给 LLM。"""
    return re.sub(r"\s+", " ", text or "").strip()


def _snippet(text: str, limit: int = _SNIPPET_MAX) -> str:
    s = _compress(text)
    return s if len(s) <= limit else s[:limit] + "…"


def _best_per_article(hits: list[dict]) -> list[dict]:
    """按 article_id 去重，同篇取最高分块；结果按分数降序（最相关在前）。"""
    best: dict[int, dict] = {}
    for h in hits:
        aid = h["article_id"]
        if aid not in best or h["score"] > best[aid]["score"]:
            best[aid] = h
    return sorted(best.values(), key=lambda h: h["score"], reverse=True)


def _no_hit() -> dict:
    return {"answer": _NO_HIT_MSG, "citations": [], "mode": "no_hit"}


def _build_citations(refs: list[dict]) -> list[dict]:
    return [
        {
            "article_id": r["article_id"],
            "title": f"知识 #{r['article_id']}",  # 占位，M9.3 由 Java 回填真实标题
            "score": r["score"],
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
    question: str, top_k: int = 5, allowed_article_ids: list[int] | None = None
) -> dict:
    """检索增强问答，返回 {answer, citations, mode}。

    mode：``llm``（LLM 总结）/ ``extractive``（抽取式兜底）/ ``no_hit``（空问题或无相关知识）。
    """
    q = (question or "").strip()
    if not q:
        return _no_hit()

    qvec, _ = embedding.embed_query(q)
    hits = get_vector_store().query(qvec, top_k=top_k, allowed_article_ids=allowed_article_ids)
    # 相关性下限过滤：丢弃明显不相关命中（叠加 LLM「资料不足就说没有」双重防臆造）
    hits = [h for h in hits if h["score"] >= settings.qa_min_score]
    if not hits:
        return _no_hit()

    refs = _best_per_article(hits)
    citations = _build_citations(refs)

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
