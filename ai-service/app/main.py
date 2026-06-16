"""kb-ai-service —— 智能客服知识库 AI 微服务（二期）。

当前为占位骨架：仅 ``/health`` 可用；``/ai/*`` 为契约占位（返回 501），
具体能力（解析 / 切分 / 向量化 / 向量检索 / LLM 问答）在二期模块 9/10 实现。

运行（从 ai-service/ 目录）::

    uvicorn app.main:app --reload
"""
from fastapi import FastAPI

from app.api import ai, health
from app.core.config import settings
from app.core.logging import logger, setup_logging

setup_logging()

app = FastAPI(
    title="智能客服知识库 · AI 服务",
    description="解析 / 切分 / 向量化 / 向量检索 / LLM 问答（二期）。当前为占位骨架。",
    version="0.1.0",
)

app.include_router(health.router)
app.include_router(ai.router)


@app.get("/", tags=["meta"])
def root() -> dict:
    return {
        "service": settings.app_name,
        "status": "placeholder",
        "phase": "二期实现；当前仅 /health 可用",
        "docs": "/docs",
    }


@app.on_event("startup")
def _on_startup() -> None:
    logger.info("%s 启动（占位骨架，env=%s）", settings.app_name, settings.env)


if __name__ == "__main__":
    import uvicorn

    uvicorn.run("app.main:app", host="127.0.0.1", port=8000, reload=True)
