"""AI 能力接口（模块 9）。

M9.1 已落地索引/检索地基（纯 Python，无 LLM 依赖）：
- POST /ai/parse        附件 → 文本块（解析 + 切分）
- POST /ai/embed        文本块 → 归一化向量（忠于既有契约，便于取原始向量/测试）
- POST /ai/index        向量化并入库（解析→切分→embed→写向量库），知识上线时由 Java 调
- POST /ai/index/remove 移除某知识的全部向量块，知识下线时由 Java 调
- POST /ai/qa           检索增强问答（向量检索 + ms-agent LLM 总结 / 抽取式兜底）

M9.4 起新增 LLM 运行时配置下发（管理员在 Qt 客户端配置，经 Java 下发，无需重启）：
- POST /ai/llm/config   应用 Java 下发的配置（改 settings + 落盘 + 重置单例）
- GET  /ai/llm/config   当前配置状态（无 api_key）
- POST /ai/llm/test     用候选值做极短生成，探测连通性（不落库不改单例）

这些 schema 即 Java 编排层与本服务之间的契约（见 app/schemas/ai.py）。
权限与上线状态过滤由 Java 编排层完成（传 allowed_article_ids）。
"""
import time

from fastapi import APIRouter, HTTPException, status

from app.core.config import settings
from app.core.logging import logger
from app.schemas.ai import (
    Chunk,
    Citation,
    EmbedRequest,
    EmbedResponse,
    IndexRemoveRequest,
    IndexRequest,
    IndexResponse,
    LlmConfigPush,
    LlmConfigStatus,
    LlmTestRequest,
    LlmTestResponse,
    ParseRequest,
    ParseResponse,
    QaRequest,
    QaResponse,
    RecommendMatchRequest,
    RecommendMatchResponse,
)
from app.services import chunker, embedding, llm, parser, qa, recommend
from app.services.vector_store import get_vector_store

router = APIRouter(prefix="/ai", tags=["ai"])


def _parse_to_chunks(file_path: str) -> list[str]:
    """解析附件并切分；把底层异常翻译为合适的 HTTP 状态码。"""
    try:
        text = parser.extract_text(file_path)
    except FileNotFoundError as e:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail=str(e))
    except ValueError as e:
        raise HTTPException(status_code=status.HTTP_400_BAD_REQUEST, detail=str(e))
    except Exception as e:  # 解析库内部错误
        logger.exception("解析附件失败：%s", file_path)
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
                            detail=f"解析失败：{e}")
    return chunker.chunk_text(text)


@router.post("/parse", response_model=ParseResponse)
def parse(req: ParseRequest) -> ParseResponse:
    """附件 → 文本块。触发点：知识上线时由 Java 发起。"""
    if not req.file_path:
        raise HTTPException(status_code=status.HTTP_400_BAD_REQUEST, detail="缺少 file_path")
    chunks = _parse_to_chunks(req.file_path)
    return ParseResponse(chunks=[Chunk(seq=i, text=t) for i, t in enumerate(chunks)])


@router.post("/embed", response_model=EmbedResponse)
def embed(req: EmbedRequest) -> EmbedResponse:
    """文本块 → 归一化向量（不写库；写库走 /ai/index）。"""
    vectors, dim = embedding.embed_texts(req.texts)
    return EmbedResponse(vectors=vectors, dim=dim)


@router.post("/index", response_model=IndexResponse)
def index(req: IndexRequest) -> IndexResponse:
    """向量化并入库：有 file_path 则解析切分，否则用 texts；embed 后写向量库。"""
    if req.file_path:
        chunks = _parse_to_chunks(req.file_path)
    elif req.texts:
        chunks = [t for t in (s.strip() for s in req.texts) if t]
    else:
        raise HTTPException(status_code=status.HTTP_400_BAD_REQUEST,
                            detail="file_path 与 texts 至少提供其一")

    vectors, dim = embedding.embed_texts(chunks)
    count = get_vector_store().upsert(req.article_id, chunks, vectors)
    return IndexResponse(article_id=req.article_id, chunk_count=count, dim=dim)


@router.post("/index/remove")
def index_remove(req: IndexRemoveRequest) -> dict:
    """移除某知识的全部向量块。触发点：知识下线/删除时由 Java 发起。"""
    removed = get_vector_store().remove_article(req.article_id)
    return {"article_id": req.article_id, "removed": removed}


@router.post("/qa", response_model=QaResponse)
def qa_endpoint(req: QaRequest) -> QaResponse:
    """问题 + topK → 答案 + 来源引用（向量检索 + LLM 总结，无 key 走抽取式兜底）。

    检索/向量库异常照常上抛 500；LLM 异常已在 qa.answer 内兜底，不会冒泡。
    """
    result = qa.answer(
        req.question, top_k=req.top_k, allowed_article_ids=req.allowed_article_ids
    )
    return QaResponse(
        answer=result["answer"],
        citations=[Citation(**c) for c in result["citations"]],
        mode=result["mode"],
    )


@router.post("/recommend/match", response_model=RecommendMatchResponse)
def recommend_match(req: RecommendMatchRequest) -> RecommendMatchResponse:
    """用户画像 vs 标签/文章向量相似度（bge，无 LLM）。"""
    segments = [{"text": s.text, "weight": s.weight} for s in req.profile_segments]
    if not segments and not (req.profile_text and req.profile_text.strip()):
        return RecommendMatchResponse()
    tag_items = [{"id": t.id, "name": t.name} for t in req.tag_items]
    article_items = [{"id": a.id, "text": a.text} for a in req.article_items]
    result = recommend.match_profile(
        req.profile_text or "",
        tag_items,
        article_items,
        top_tags=req.top_tags,
        top_articles=req.top_articles,
        profile_segments=segments or None,
    )
    return RecommendMatchResponse(**result)


# ——— LLM 运行时配置（M9.4）———

def _llm_status() -> LlmConfigStatus:
    """当前 LLM 配置状态——不含 api_key。"""
    return LlmConfigStatus(
        configured=llm.is_configured(),
        base_url=settings.llm_base_url,
        model=settings.llm_model,
        temperature=settings.llm_temperature,
        max_tokens=settings.llm_max_tokens,
    )


@router.post("/llm/config", response_model=LlmConfigStatus)
def llm_config_apply(req: LlmConfigPush) -> LlmConfigStatus:
    """应用 Java 下发的 LLM 配置：改 settings + 落盘 + 重置单例；返回应用后状态（无 key）。"""
    llm.apply_config(req.model_dump())
    return _llm_status()


@router.get("/llm/config", response_model=LlmConfigStatus)
def llm_config_get() -> LlmConfigStatus:
    """当前 LLM 配置状态（无 api_key）。供 Java 核对下发是否生效。"""
    return _llm_status()


@router.post("/llm/test", response_model=LlmTestResponse)
def llm_config_test(req: LlmTestRequest) -> LlmTestResponse:
    """用候选值做一次极短生成探测连通性；成功/失败都 200 返回，由 ok 字段区分。"""
    t0 = time.perf_counter()
    try:
        text = llm.test_config(req.model_dump())
    except Exception as e:  # 连通失败：把错误原因带回前端，不抛 500
        logger.warning("LLM 连通测试失败：%s", e)
        return LlmTestResponse(ok=False, message=str(e)[:300],
                               latency_ms=int((time.perf_counter() - t0) * 1000))
    return LlmTestResponse(ok=True, message=(text or "(空响应)")[:300],
                           latency_ms=int((time.perf_counter() - t0) * 1000))
