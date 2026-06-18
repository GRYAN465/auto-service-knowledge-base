"""kb-ai-service —— 智能客服知识库 AI 微服务（二期 · 模块 9）。

M9.1 索引/检索地基 + M9.2 智能问答已落地：``/ai/parse``、``/ai/embed``、
``/ai/index``、``/ai/index/remove`` 负责解析/切分/向量化/写检索向量库；``/ai/qa``
做检索增强问答（向量检索 + ms-agent 驱动 OpenAI 兼容 LLM 总结，无 key 抽取式兜底）。

运行（从 ai-service/ 目录）::

    uvicorn app.main:app --reload
"""
from fastapi import FastAPI

from app.api import ai, health
from app.core.config import settings
from app.core.logging import logger, setup_logging
from app.services.vector_store import get_vector_store

setup_logging()

app = FastAPI(
    title="智能客服知识库 · AI 服务",
    description="解析 / 切分 / 向量化 / 向量检索 / LLM 问答（模块 9）。M9.4 LLM 运行时配置下发已落地。",
    version="0.4.1",
)

app.include_router(health.router)
app.include_router(ai.router)


@app.get("/", tags=["meta"])
def root() -> dict:
    return {
        "service": settings.app_name,
        "status": "ok",
        "phase": "M9.4 LLM 运行时配置下发（索引/检索 + /ai/qa + /ai/llm/config）+ 启动自清向量库",
        "docs": "/docs",
    }


@app.on_event("startup")
def _on_startup() -> None:
    logger.info("%s 启动（env=%s，向量库=%s）", settings.app_name, settings.env,
                settings.vector_store_dir)
    if settings.rebuild_on_startup:
        vs = get_vector_store()
        removed = vs.clear_all()
        logger.info("启动自清：已清空 %d 条向量（Java 启动后将全量重建）", removed)
    else:
        logger.info("rebuild_on_startup=false，保留既有向量数据")


if __name__ == "__main__":
    import uvicorn

    uvicorn.run("app.main:app", host="127.0.0.1", port=8000, reload=True)
