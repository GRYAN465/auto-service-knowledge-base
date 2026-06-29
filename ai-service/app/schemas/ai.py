"""Java 编排层 ↔ FastAPI 的接口契约（pydantic 模型即契约定义）。二期落地。"""
<<<<<<< ai-service/app/schemas/ai.py
from pydantic import BaseModel, Field, field_validator
=======
from typing import Literal


>>>>>>> ai-service/app/schemas/ai.py

KnowledgeType = Literal["SCRIPT", "TRAIN", "PRODUCT", "OFFICE"]


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


# ——— /ai/index：向量化并入库（解析→切分→embed→写向量库）———
class IndexRequest(BaseModel):
    article_id: int = Field(..., description="所属知识 ID（kb_article.id），向量库写入单位")
    knowledge_type: KnowledgeType = Field(..., description="知识类型")
    file_path: str | None = Field(
        None, description="附件相对/绝对路径；提供则由本服务解析切分"
    )
    texts: list[str] | None = Field(
        None, description="已切好的文本块；与 file_path 二选一（优先 file_path）"
    )


class IndexResponse(BaseModel):
    article_id: int
    chunk_count: int = Field(0, description="实际入库块数")
    dim: int = Field(0, description="向量维度")


class IndexRemoveRequest(BaseModel):
    article_id: int = Field(..., description="下线/删除时移除该知识的全部向量块")
    knowledge_type: KnowledgeType = Field(..., description="知识类型")


# ——— /ai/qa：检索增强问答 ———
class Citation(BaseModel):
    article_id: int
    title: str  # FastAPI 侧占位「知识 #{id}」，由 Java 编排层回填真实标题
    score: float
    snippet: str | None = Field(None, description="命中原文片段（供前端展示/核对引用）")


class QaRequest(BaseModel):
    question: str
    knowledge_type: KnowledgeType | None = Field(None, description="知识类型；None 则默认搜索话术库")
    top_k: int = Field(5, description="召回条数")
    # 权限与上线状态过滤由 Java 编排层完成，传入允许检索的知识范围
    allowed_article_ids: list[int] | None = Field(
        None, description="允许命中的知识 ID（None=不限，由上游保证已过滤）"
    )


class QaResponse(BaseModel):
    answer: str
    citations: list[Citation] = []
    mode: str = Field(
        "extractive", description="答案来源：llm（LLM 总结）/ extractive（抽取式兜底）/ no_hit（无相关知识）"
    )


# ——— /ai/llm/config、/ai/llm/test：LLM 运行时配置下发与连通性测试（M9.4）———
# Java 编排层为权威源，把管理员在客户端填写的配置下发到此，本服务应用 + 落盘自存。
class LlmConfigPush(BaseModel):
    base_url: str = Field("", description="OpenAI 兼容端点(通常以 /v1 结尾)")
    api_key: str = Field("", description="API 密钥")
    model: str = Field("", description="模型名")
    temperature: float = Field(0.2, description="采样温度")
    max_tokens: int = Field(1024, description="最大生成 token 数")


class LlmConfigStatus(BaseModel):
    """GET /ai/llm/config 回执：仅暴露非敏感状态，绝不回传 api_key。"""
    configured: bool = Field(..., description="base_url/api_key/model 三件套是否齐备")
    base_url: str = ""
    model: str = ""
    temperature: float = 0.2
    max_tokens: int = 1024


class LlmTestRequest(BaseModel):
    base_url: str = Field("", description="待测端点")
    api_key: str = Field("", description="待测密钥")
    model: str = Field("", description="待测模型")
    temperature: float = Field(0.2, description="采样温度")
    max_tokens: int = Field(64, description="测试用较小的 max_tokens")


class LlmTestResponse(BaseModel):
    ok: bool = Field(..., description="是否连通并成功生成")
    message: str = Field("", description="成功为模型返回文本片段，失败为错误原因")
    latency_ms: int = Field(0, description="一次极短生成的往返耗时(毫秒)")


# ——— /ai/recommend/match：首页推送画像相似度 ———
class RecommendTagItem(BaseModel):
    id: int
    name: str


class RecommendArticleItem(BaseModel):
    id: int
    text: str = Field(..., description="标题+摘要+标签名，用于向量化")


class ProfileSegment(BaseModel):
    text: str = Field(..., description="画像分段文本")
    weight: float = Field(..., ge=0, description="融合权值")


class RecommendMatchRequest(BaseModel):
    profile_text: str = Field("", description="兼容旧版单段画像；优先 profile_segments")
    profile_segments: list[ProfileSegment] = Field(default_factory=list, description="分层画像+权值")
    tag_items: list[RecommendTagItem] = []
    article_items: list[RecommendArticleItem] = []
    top_tags: int = Field(5, description="返回兴趣标签数")
    top_articles: int = Field(30, description="返回相似文章数（供编排层参考）")

    @field_validator("profile_text", mode="before")
    @classmethod
    def _coerce_profile_text(cls, v: object) -> str:
        return "" if v is None else str(v)


class RecommendScoreItem(BaseModel):
    id: int
    score: float


class RecommendTagScoreItem(BaseModel):
    id: int
    name: str
    score: float


class RecommendMatchResponse(BaseModel):
    top_tags: list[RecommendTagScoreItem] = []
    top_articles: list[RecommendScoreItem] = []
