"""向量库：抽象接口 + Chroma 实现（进程内嵌、落盘）。

检索全走 VectorStore 接口，Chroma 仅是其一实现，后续可换（pgvector/qdrant…）。
块 id 规则：a{article_id}-c{seq}（稳定可重建）；metadata 带 article_id/seq/status。
下线即删整篇、上线再重建——避开 metadata 原地更新的复杂度。
"""
from __future__ import annotations

from abc import ABC, abstractmethod
from pathlib import Path

from app.core.config import settings
from app.core.logging import logger


class VectorStore(ABC):
    """向量库抽象。article 为单位写入/删除，向量检索带范围与状态过滤。"""

    @abstractmethod
    def upsert(self, article_id: int, chunks: list[str], vectors: list[list[float]]) -> int:
        """写入/覆盖某篇知识的全部块，返回写入条数。"""

    @abstractmethod
    def query(
        self,
        qvec: list[float],
        top_k: int = 5,
        allowed_article_ids: list[int] | None = None,
        status: str = "online",
    ) -> list[dict]:
        """向量检索，返回 [{article_id, seq, text, score}]（score 为余弦相似度，越大越相关）。"""

    @abstractmethod
    def remove_article(self, article_id: int) -> int:
        """删除某篇知识的全部块，返回删除条数。"""


class ChromaVectorStore(VectorStore):
    def __init__(self, persist_dir: str, collection_name: str):
        import chromadb
        from chromadb.config import Settings as ChromaSettings

        Path(persist_dir).mkdir(parents=True, exist_ok=True)
        self._client = chromadb.PersistentClient(
            path=persist_dir,
            settings=ChromaSettings(anonymized_telemetry=False),
        )
        # 余弦空间：归一化向量下 distance≈1-cos，score=1-distance 即余弦相似度
        self._col = self._client.get_or_create_collection(
            name=collection_name,
            metadata={"hnsw:space": "cosine"},
        )
        logger.info("Chroma 就绪：dir=%s collection=%s 现有块数=%d",
                    persist_dir, collection_name, self._col.count())

    def upsert(self, article_id: int, chunks: list[str], vectors: list[list[float]]) -> int:
        # 先删旧块再写，保证重建干净（块数变化也不残留）
        self.remove_article(article_id)
        if not chunks:
            return 0
        ids = [f"a{article_id}-c{seq}" for seq in range(len(chunks))]
        metas = [
            {"article_id": int(article_id), "seq": seq, "status": "online"}
            for seq in range(len(chunks))
        ]
        self._col.upsert(ids=ids, documents=chunks, embeddings=vectors, metadatas=metas)
        logger.info("upsert article=%d 块数=%d", article_id, len(ids))
        return len(ids)

    def query(
        self,
        qvec: list[float],
        top_k: int = 5,
        allowed_article_ids: list[int] | None = None,
        status: str = "online",
    ) -> list[dict]:
        where = self._build_where(allowed_article_ids, status)
        # allowed_article_ids 为空列表表示「无任何可命中范围」，直接空结果
        if allowed_article_ids is not None and len(allowed_article_ids) == 0:
            return []
        res = self._col.query(
            query_embeddings=[qvec],
            n_results=max(1, top_k),
            where=where,
        )
        ids = res.get("ids", [[]])[0]
        docs = res.get("documents", [[]])[0]
        metas = res.get("metadatas", [[]])[0]
        dists = res.get("distances", [[]])[0]
        out: list[dict] = []
        for i in range(len(ids)):
            meta = metas[i] or {}
            out.append({
                "article_id": int(meta.get("article_id", 0)),
                "seq": int(meta.get("seq", 0)),
                "text": docs[i],
                "score": round(1.0 - float(dists[i]), 4),
            })
        return out

    def remove_article(self, article_id: int) -> int:
        before = self._col.count()
        self._col.delete(where={"article_id": int(article_id)})
        removed = before - self._col.count()
        if removed:
            logger.info("remove article=%d 删除块数=%d", article_id, removed)
        return removed

    def clear_all(self) -> int:
        """清空 collection 内全部向量块（启动时重置，配合 Java 全量重建）。返回清掉的块数。"""
        before = self._col.count()
        if before == 0:
            logger.info("向量库已为空，跳过清空")
            return 0
        # Chroma 没有 truncate；用 delete(where={}) 清空全collection
        self._col.delete(where={})
        after = self._col.count()
        removed = before - after
        logger.info("向量库已清空：删除块数=%d 剩余=%d", removed, after)
        return removed

    @staticmethod
    def _build_where(allowed_article_ids: list[int] | None, status: str) -> dict | None:
        conds = []
        if status:
            conds.append({"status": status})
        if allowed_article_ids:
            conds.append({"article_id": {"$in": [int(a) for a in allowed_article_ids]}})
        if not conds:
            return None
        return conds[0] if len(conds) == 1 else {"$and": conds}


_store: VectorStore | None = None


def get_vector_store() -> VectorStore:
    """进程内单例，复用 Chroma client。"""
    global _store
    if _store is None:
        _store = ChromaVectorStore(settings.vector_store_dir, settings.collection_name)
    return _store
