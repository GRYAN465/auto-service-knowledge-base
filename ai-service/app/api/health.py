"""健康检查：脚手架冒烟用。"""
from fastapi import APIRouter

router = APIRouter(tags=["health"])


@router.get("/health")
def health() -> dict:
    return {"status": "UP", "service": "kb-ai-service"}
