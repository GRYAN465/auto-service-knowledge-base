"""Test multi-collection VectorStore factory."""
import os
import tempfile
from pathlib import Path

# Isolate with temp dir BEFORE importing app modules
_tmp = Path(tempfile.mkdtemp(prefix="kb-ai-test-vs-"))
os.environ["KB_AI_VECTOR_STORE_DIR"] = str(_tmp / "vector_store")

from app.services.vector_store import (
    _KNOWLEDGE_TYPES,
    _collection_name,
    clear_all,
    get_vector_store,
)


def test_collection_name():
    assert _collection_name("SCRIPT") == "kb_knowledge_script"
    assert _collection_name("TRAIN") == "kb_knowledge_train"
    assert _collection_name("PRODUCT") == "kb_knowledge_product"
    assert _collection_name("OFFICE") == "kb_knowledge_office"


def test_get_store_by_type_returns_different_stores():
    s1 = get_vector_store("SCRIPT")
    s2 = get_vector_store("TRAIN")
    assert s1 is not s2, "不同 knowledge_type 应返回不同 VectorStore 实例"


def test_get_store_none_returns_default():
    store = get_vector_store()
    assert store is not None


def test_upsert_and_query_scoped():
    import numpy as np

    s = get_vector_store("SCRIPT")
    # Clean before test
    s.clear_all()

    dim = 512
    vec_a = np.random.randn(dim).tolist()
    vec_b = np.random.randn(dim).tolist()

    s.upsert(1001, ["话术块A", "话术块B"], [vec_a, vec_a])
    s.upsert(1002, ["话术块C"], [vec_b])

    # SCRIPT store should find its own data
    hits = s.query(vec_a, top_k=5)
    assert len(hits) >= 1

    # TRAIN store should be empty
    t = get_vector_store("TRAIN")
    t.clear_all()
    hits_train = t.query(vec_a, top_k=5)
    assert len(hits_train) == 0, "TRAIN collection 应与 SCRIPT 隔离"


def test_clear_all_clears_all_types():
    import numpy as np

    dim = 512
    # Write one vector into each type
    for kt in _KNOWLEDGE_TYPES:
        s = get_vector_store(kt)
        s.clear_all()
        s.upsert(9999, ["test"], [np.random.randn(dim).tolist()])

    total = clear_all()
    assert total > 0

    # Verify all empty
    for kt in _KNOWLEDGE_TYPES:
        s = get_vector_store(kt)
        hits = s.query(np.random.randn(dim).tolist(), top_k=5)
        assert len(hits) == 0, f"{kt} 应已被清空"
