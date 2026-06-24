"""健康检查：脚手架冒烟用。"""
from fastapi import APIRouter

from app.services.vector_store import get_vector_store

router = APIRouter(tags=["health"])


@router.get("/health")
def health() -> dict:
    try:
        chunks = get_vector_store().count()
    except Exception:
        chunks = -1
    return {
        "status": "UP",
        "service": "kb-ai-service",
        "vector_chunks": chunks,
    }
