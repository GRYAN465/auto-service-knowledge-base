"""Embedding：本地 bge-zh（CPU），懒加载单例。

bge-small-zh-v1.5：512 维，L2 归一化后做余弦检索。
- 文档块（passage）直接编码，不加指令前缀。
- 查询（query）建议加检索指令前缀（见 embed_query），M9.2 问答时用。

本地模型缺失时自动从 ModelScope 下载到 models/bge-small-zh-v1.5/，
下载失败再回退 HuggingFace 缓存。
"""
from __future__ import annotations

from pathlib import Path

from app.core.config import settings
from app.core.logging import logger

# bge-zh 检索指令：仅用于「查询」侧，提升短查询→长文档的召回
_QUERY_INSTRUCTION = "为这个句子生成表示以用于检索相关文章："
# ModelScope 模型 ID → 本地目录下载
_MODELSCOPE_MODEL_ID = "AI-ModelScope/bge-small-zh-v1.5"
# 兜底：HF 模型 ID（ModelScope 下载失败时用）
_FALLBACK_MODEL_ID = "BAAI/bge-small-zh-v1.5"

_model = None  # 懒加载单例


def _download_model_if_missing(target_dir: str) -> bool:
    """通过 ModelScope 下载 bge-small-zh-v1.5 到 target_dir；成功返回 True。"""
    try:
        from modelscope import snapshot_download

        logger.info("正在从 ModelScope 下载 embedding 模型到 %s（约 95MB）…", target_dir)
        snapshot_download(_MODELSCOPE_MODEL_ID, local_dir=target_dir)
        logger.info("模型下载完成：%s", target_dir)
        return True
    except Exception as e:
        logger.warning("ModelScope 下载失败：%s；将回退到 HuggingFace", e)
        return False


def _get_model():
    global _model
    if _model is None:
        from sentence_transformers import SentenceTransformer

        target = settings.embedding_model
        if not Path(target).exists():
            ok = _download_model_if_missing(target)
            if not ok:
                logger.warning("本地 embedding 模型不存在且下载失败，回退按模型名联网加载 %s",
                               _FALLBACK_MODEL_ID)
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
    vectors, dim = embed_queries([query])
    return vectors[0], dim


def embed_queries(queries: list[str]) -> tuple[list[list[float]], int]:
    """多条查询批量 embedding（带 bge 检索指令前缀）。"""
    if not queries:
        return [], 0
    return embed_texts([_QUERY_INSTRUCTION + q for q in queries])


def warmup() -> None:
    """启动预热：加载 bge 并完成一次 dummy embed，避免首请求长时间阻塞。"""
    _, dim = embed_texts(["预热"])
    logger.info("embedding 预热完成（dim=%d）", dim)
