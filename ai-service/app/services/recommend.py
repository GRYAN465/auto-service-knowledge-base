"""首页个性化推荐：用户画像向量 vs 标签/文章向量相似度（bge，无 LLM）。"""

from __future__ import annotations



import math



from app.services import embedding



_tag_vec_cache: dict[int, list[float]] = {}

_tag_name_sig: tuple[tuple[int, str], ...] = ()





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





def _fuse_profile_vector(profile_text: str, profile_segments: list[dict] | None) -> list[float]:

    """多段画像按权值加权融合后 L2 归一化。"""

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



    acc: list[float] | None = None

    for text, weight in pairs:

        vec, _ = embedding.embed_query(text)

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

        texts = [str(a.get("text") or "") for a in article_items]

        vecs, _ = embedding.embed_texts(texts)

        for item, vec in zip(article_items, vecs):

            if not item.get("text"):

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

