"""首页个性化推荐：用户画像向量 vs 标签/文章向量相似度（bge，无 LLM）。"""

from __future__ import annotations

import math
from collections import OrderedDict

from app.services import embedding

_tag_vec_cache: dict[int, list[float]] = {}
_tag_name_sig: tuple[tuple[int, str], ...] = ()

# article_id → (text, vector)；文本变更时自动失效；LRU 上限防内存膨胀
_article_vec_cache: OrderedDict[int, tuple[str, list[float]]] = OrderedDict()
_ARTICLE_CACHE_MAX = 2000


def _dot(a: list[float], b: list[float]) -> float:
    return float(sum(x * y for x, y in zip(a, b)))


def _l2_normalize(vec: list[float]) -> list[float]:
    norm = math.sqrt(sum(x * x for x in vec))
    if norm <= 0:
        return vec
    return [x / norm for x in vec]


def _top_k_scored(items: list[dict], k: int) -> list[dict]:
    items.sort(key=lambda x: x["score"], reverse=True)
    return items[: max(0, k)]


def _trim_article_cache() -> None:
    while len(_article_vec_cache) > _ARTICLE_CACHE_MAX:
        _article_vec_cache.popitem(last=False)


def _tag_vectors(tag_items: list[dict]) -> list[list[float]]:
    """标签名向量；标签集变化时刷新内存缓存。"""
    global _tag_vec_cache, _tag_name_sig
    sig = tuple((int(t["id"]), str(t["name"])) for t in tag_items)
    if sig != _tag_name_sig:
        names = [str(t["name"]) for t in tag_items]
        vecs, _ = embedding.embed_texts(names)
        _tag_vec_cache = {int(t["id"]): v for t, v in zip(tag_items, vecs)}
        _tag_name_sig = sig
    return [_tag_vec_cache[int(t["id"])] for t in tag_items]


def _article_vectors(article_items: list[dict]) -> list[list[float] | None]:
    """文章文本向量；按 article_id + 文本内容 LRU 缓存，仅对未命中 batch embed。"""
    vecs_out: list[list[float] | None] = [None] * len(article_items)
    misses: list[tuple[int, int, str]] = []

    for idx, item in enumerate(article_items):
        text = str(item.get("text") or "")
        if not text:
            continue
        aid = int(item["id"])
        cached = _article_vec_cache.get(aid)
        if cached is not None and cached[0] == text:
            _article_vec_cache.move_to_end(aid)
            vecs_out[idx] = cached[1]
        else:
            misses.append((idx, aid, text))

    if misses:
        batch_vecs, _ = embedding.embed_texts([t for _, _, t in misses])
        for (idx, aid, text), vec in zip(misses, batch_vecs):
            _article_vec_cache[aid] = (text, vec)
            _article_vec_cache.move_to_end(aid)
            vecs_out[idx] = vec
        _trim_article_cache()

    return vecs_out


def _fuse_profile_vector(profile_text: str, profile_segments: list[dict] | None) -> list[float]:
    """多段画像按权值加权融合后 L2 归一化（批量 embed_query）。"""
    pairs: list[tuple[str, float]] = []
    if profile_segments:
        for seg in profile_segments:
            text = str(seg.get("text") or "").strip()
            weight = float(seg.get("weight") or 0.0)
            if text and weight > 0:
                pairs.append((text, weight))
    if not pairs and profile_text.strip():
        pairs.append((profile_text.strip(), 1.0))
    if not pairs:
        return []

    total_w = sum(w for _, w in pairs)
    if total_w <= 0:
        return []

    texts = [t for t, _ in pairs]
    seg_vecs, _ = embedding.embed_queries(texts)
    acc: list[float] | None = None
    for (_, weight), vec in zip(pairs, seg_vecs):
        if not vec:
            continue
        scaled = [v * weight / total_w for v in vec]
        if acc is None:
            acc = scaled
        else:
            acc = [a + b for a, b in zip(acc, scaled)]

    return _l2_normalize(acc) if acc else []


def match_profile(
    profile_text: str,
    tag_items: list[dict],
    article_items: list[dict],
    top_tags: int = 5,
    top_articles: int = 30,
    profile_segments: list[dict] | None = None,
) -> dict:
    """profile → 与标签/文章余弦相似度 TopK（向量已 L2 归一化）。"""
    profile_vec = _fuse_profile_vector(profile_text, profile_segments)
    if not profile_vec:
        return {"top_tags": [], "top_articles": []}

    tag_scores: list[dict] = []
    if tag_items:
        for item, vec in zip(tag_items, _tag_vectors(tag_items)):
            tag_scores.append(
                {
                    "id": int(item["id"]),
                    "name": str(item["name"]),
                    "score": _dot(profile_vec, vec),
                }
            )

    article_scores: list[dict] = []
    if article_items:
        for item, vec in zip(article_items, _article_vectors(article_items)):
            if not item.get("text") or vec is None:
                continue
            article_scores.append(
                {
                    "id": int(item["id"]),
                    "score": _dot(profile_vec, vec),
                }
            )

    return {
        "top_tags": _top_k_scored(tag_scores, top_tags),
        "top_articles": _top_k_scored(article_scores, top_articles),
    }


def invalidate_article_cache(article_id: int | None = None) -> None:
    """索引更新或文章变更时可调用；article_id=None 时清空全部文章缓存。"""
    if article_id is None:
        _article_vec_cache.clear()
    else:
        _article_vec_cache.pop(int(article_id), None)
