"""Java 编排层 ↔ FastAPI 的接口契约（pydantic 模型即契约定义）。二期落地。"""
from pydantic import BaseModel, Field


# ——— /ai/parse：附件解析切分 ———
class ParseRequest(BaseModel):
    article_id: int | None = Field(None, description="所属知识 ID（kb_article.id）")
    file_id: int | None = Field(None, description="附件 ID（kb_attachment.id）")
    file_path: str | None = Field(None, description="附件存储路径")


class Chunk(BaseModel):
    seq: int = Field(..., description="切分序号")
    text: str


class ParseResponse(BaseModel):
    chunks: list[Chunk] = []


# ——— /ai/embed：文本向量化 ———
class EmbedRequest(BaseModel):
    texts: list[str] = Field(..., description="待向量化文本块")


class EmbedResponse(BaseModel):
    vectors: list[list[float]] = []
    dim: int = 0


# ——— /ai/qa：检索增强问答 ———
class Citation(BaseModel):
    article_id: int
    title: str
    score: float


class QaRequest(BaseModel):
    question: str
    top_k: int = Field(5, description="召回条数")
    # 权限与上线状态过滤由 Java 编排层完成，传入允许检索的知识范围
    allowed_article_ids: list[int] | None = Field(
        None, description="允许命中的知识 ID（None=不限，由上游保证已过滤）"
    )


class QaResponse(BaseModel):
    answer: str
    citations: list[Citation] = []
