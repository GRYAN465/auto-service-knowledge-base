"""精排：本地 bge-reranker 交叉编码器（CPU），懒加载单例。

向量检索是双塔（query / passage 各自编码再算余弦），快但召回粗；交叉编码器把
(query, chunk) 拼到一起过一遍模型直接打相关分，精度高、代价大。检索增强里的标准做法：
向量先宽召回（retrieve_k 条），再用本模块精排取前几条喂 LLM，显著提升 Top-K 精度。

graceful degradation（对齐 embedding/llm 的风格）：模型缺失自动从 ModelScope 下载到
``models/bge-reranker-base/``，下载失败回退 HuggingFace；若禁用或彻底加载失败，``rerank``
原样按向量分排序返回——精排只是锦上添花，永不让问答因精排失败而 500。
"""
from __future__ import annotations

from pathlib import Path

from app.core.config import settings
from app.core.logging import logger

# ModelScope 模型 ID → 本地目录下载；与 embedding 同源（BAAI 官方仓库，中英双语）
_MODELSCOPE_MODEL_ID = "BAAI/bge-reranker-base"
# 兜底：HF 模型 ID（ModelScope 下载失败时按名联网加载）
_FALLBACK_MODEL_ID = "BAAI/bge-reranker-base"

_model = None          # 懒加载单例
_load_failed = False   # 加载失败后置位，避免每次问答反复重试下载


def is_enabled() -> bool:
    """配置是否开启精排（KB_AI_RERANK_ENABLED）。"""
    return settings.rerank_enabled


def _download_model_if_missing(target_dir: str) -> bool:
    """通过 ModelScope 下载 bge-reranker-base 到 target_dir；成功返回 True。"""
    try:
        from modelscope import snapshot_download

        logger.info("正在从 ModelScope 下载 reranker 模型到 %s（约 1.1GB，首次较慢）…", target_dir)
        snapshot_download(_MODELSCOPE_MODEL_ID, local_dir=target_dir)
        logger.info("reranker 模型下载完成：%s", target_dir)
        return True
    except Exception as e:
        logger.warning("ModelScope 下载 reranker 失败：%s；将回退 HuggingFace", e)
        return False


def _get_model():
    """构造并缓存交叉编码器单例；任一步失败则置 _load_failed，返回 None（问答退回向量分）。"""
    global _model, _load_failed
    if _model is not None or _load_failed:
        return _model
    try:
        from sentence_transformers import CrossEncoder

        target = settings.rerank_model
        if not Path(target).exists():
            if not _download_model_if_missing(target):
                logger.warning("本地 reranker 模型不存在且下载失败，回退按模型名联网加载 %s",
                               _FALLBACK_MODEL_ID)
                target = _FALLBACK_MODEL_ID
        logger.info("加载 reranker 模型：%s（device=%s）", target, settings.rerank_device)
        _model = CrossEncoder(target, device=settings.rerank_device)
    except Exception:
        logger.exception("reranker 加载失败，问答将退回向量分排序（不影响可用性）")
        _load_failed = True
    return _model


def is_available() -> bool:
    """精排是否真正可用：已启用且模型可加载（会触发懒加载）。"""
    return is_enabled() and _get_model() is not None


def _sigmoid(arr):
    import numpy as np

    return 1.0 / (1.0 + np.exp(-arr))


def rerank(query: str, hits: list[dict]) -> list[dict]:
    """对召回 hits 用交叉编码器精排：写回 ``hit['rerank_score']`` 并按其降序返回。

    精排不可用（禁用 / 加载失败）时**不标注** rerank_score，仅按既有向量分 ``score`` 降序返回——
    调用方据 ``rerank_score`` 是否存在判断该用哪套相关性阈值。
    """
    if not hits:
        return []
    model = _get_model()
    if model is None:  # 未启用 / 加载失败：退回向量分排序
        return sorted(hits, key=lambda h: h["score"], reverse=True)

    import numpy as np

    try:
        pairs = [[query, h["text"]] for h in hits]
        raw = np.asarray(
            model.predict(pairs, convert_to_numpy=True, show_progress_bar=False),
            dtype=float,
        ).reshape(-1)
        # bge-reranker 默认输出 logits（任意实数）；越界则 sigmoid 归一到 (0,1)，便于设阈值。
        # 若安装版本已对单标签输出套了 sigmoid（分值已在 [0,1]），则跳过，避免二次压缩失真。
        if raw.size and (raw.min() < 0.0 or raw.max() > 1.0):
            raw = _sigmoid(raw)
    except Exception:  # 精排打分阶段出错也兜底——不让问答因精排 500
        logger.exception("reranker 打分失败，本次退回向量分排序")
        return sorted(hits, key=lambda h: h["score"], reverse=True)

    for h, s in zip(hits, raw):
        h["rerank_score"] = round(float(s), 4)
    return sorted(hits, key=lambda h: h["rerank_score"], reverse=True)
