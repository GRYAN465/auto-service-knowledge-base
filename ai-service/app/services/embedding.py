"""Embedding：本地 bge-zh（CPU），懒加载单例。

bge-small-zh-v1.5：512 维，L2 归一化后做余弦检索。
- 文档块（passage）直接编码，不加指令前缀。
- 查询（query）建议加检索指令前缀（见 embed_query），M9.2 问答时用。
"""
from __future__ import annotations

from pathlib import Path

from app.core.config import settings
from app.core.logging import logger

# bge-zh 检索指令：仅用于「查询」侧，提升短查询→长文档的召回
_QUERY_INSTRUCTION = "为这个句子生成表示以用于检索相关文章："
# 本地模型缺失时的联网回退（魔搭/HF 同名）
_FALLBACK_MODEL_ID = "BAAI/bge-small-zh-v1.5"

_model = None  # 懒加载单例


def _get_model():
    global _model
    if _model is None:
        from sentence_transformers import SentenceTransformer

        target = settings.embedding_model
        if not Path(target).exists():
            logger.warning("本地 embedding 模型不存在：%s；回退按模型名联网加载 %s",
                           target, _FALLBACK_MODEL_ID)
            target = _FALLBACK_MODEL_ID
        logger.info("加载 embedding 模型：%s（device=%s）", target, settings.embedding_device)
        _model = SentenceTransformer(target, device=settings.embedding_device)
    return _model


def embed_texts(texts: list[str]) -> tuple[list[list[float]], int]:
    """文档块 → 归一化向量。返回 (vectors, dim)；空输入返回 ([], 0)。"""
    if not texts:
        return [], 0
    model = _get_model()
    arr = model.encode(
        texts,
        normalize_embeddings=True,
        convert_to_numpy=True,
        show_progress_bar=False,
    )
    return arr.tolist(), int(arr.shape[1])


def embed_query(query: str) -> tuple[list[float], int]:
    """查询 → 归一化向量（带 bge 检索指令前缀）。返回 (vector, dim)。"""
    vectors, dim = embed_texts([_QUERY_INSTRUCTION + query])
    return vectors[0], dim
