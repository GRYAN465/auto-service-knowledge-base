"""AI 能力接口（二期）。当前为契约占位：路由与 schema 已定义，逻辑返回 501。

这些 schema 即 Java 编排层与本服务之间的契约（见 app/schemas/ai.py）。
"""
from fastapi import APIRouter, HTTPException, status

from app.schemas.ai import (
    EmbedRequest,
    EmbedResponse,
    ParseRequest,
    ParseResponse,
    QaRequest,
    QaResponse,
)

router = APIRouter(prefix="/ai", tags=["ai"])

_PHASE2 = "AI 能力为二期（模块 9/10）实现，当前为占位接口。"


@router.post("/parse", response_model=ParseResponse)
def parse(req: ParseRequest) -> ParseResponse:
    """附件 → 文本块。触发点：知识上线时由 Java 发起。"""
    raise HTTPException(status_code=status.HTTP_501_NOT_IMPLEMENTED, detail=_PHASE2)


@router.post("/embed", response_model=EmbedResponse)
def embed(req: EmbedRequest) -> EmbedResponse:
    """文本块 → 向量，写入向量库。"""
    raise HTTPException(status_code=status.HTTP_501_NOT_IMPLEMENTED, detail=_PHASE2)


@router.post("/qa", response_model=QaResponse)
def qa(req: QaRequest) -> QaResponse:
    """问题 + topK → 答案 + 来源引用（向量检索 + LLM 总结）。"""
    raise HTTPException(status_code=status.HTTP_501_NOT_IMPLEMENTED, detail=_PHASE2)
