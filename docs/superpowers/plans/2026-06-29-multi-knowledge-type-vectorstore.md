# 多知识类型分库 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Split the single Chroma collection into 4 independent collections (SCRIPT/TRAIN/PRODUCT/OFFICE), with API-level `knowledge_type` routing.

**Architecture:** Extend `vector_store.py` with a factory that maps `knowledge_type` → Chroma collection name (`kb_knowledge_script`, etc.). API schemas add a required `knowledge_type` field. `qa.answer()` accepts `knowledge_type` and routes to the correct collection. Backward-compatible: `get_vector_store(None)` still returns the old default collection.

**Tech Stack:** Python 3.12, ChromaDB, FastAPI, Pydantic

## Global Constraints

- Chroma collections named `kb_knowledge_{type_lower}`
- `knowledge_type` values: `SCRIPT`, `TRAIN`, `PRODUCT`, `OFFICE`
- Existing callers using `get_vector_store()` (no args) must continue to work
- HTTP API requires `knowledge_type`; Python function `qa.answer()` uses default `"SCRIPT"` for backward compat
- No changes to ChromaVectorStore method signatures — only the factory changes

---

### Task 1: VectorStore — multi-collection factory + clear_all()

**Files:**
- Modify: `ai-service/app/services/vector_store.py` (entire file)

**Interfaces:**
- Consumes: `settings.vector_store_dir` (unchanged)
- Produces: `get_vector_store(knowledge_type: str | None = None) -> VectorStore`
- Produces: `clear_all() -> int`
- Produces: module-level `_KNOWLEDGE_TYPES: tuple[str, ...]`

- [ ] **Step 1: Write the tests**

Create `ai-service/tests/test_vector_store_multi.py`:

```python
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
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd ai-service && python -m pytest tests/test_vector_store_multi.py -v`
Expected: FAIL — ImportError (functions not yet defined)

- [ ] **Step 3: Implement the factory in `vector_store.py`**

Replace the current module-level code (after `class ChromaVectorStore`) with:

```python
_KNOWLEDGE_TYPES = ("SCRIPT", "TRAIN", "PRODUCT", "OFFICE")

_stores: dict[str, VectorStore] = {}


def _collection_name(knowledge_type: str) -> str:
    """SCRIPT → kb_knowledge_script"""
    return f"kb_knowledge_{knowledge_type.lower()}"


def get_vector_store(knowledge_type: str | None = None) -> VectorStore:
    """获取对应知识类型的向量库。

    Args:
        knowledge_type: ``SCRIPT`` / ``TRAIN`` / ``PRODUCT`` / ``OFFICE`` 之一。
                        ``None`` 返回旧版单 collection ``kb_knowledge``（向后兼容）。
    """
    if knowledge_type is None:
        return _get_or_create("kb_knowledge")
    return _get_or_create(_collection_name(knowledge_type))


def clear_all() -> int:
    """清空所有已知类型的 collection + 默认 collection。返回清除的块总数。"""
    total = 0
    for kt in _KNOWLEDGE_TYPES:
        store = _get_or_create(_collection_name(kt), cache=False)
        total += store.clear_all()
    default = _get_or_create("kb_knowledge", cache=False)
    total += default.clear_all()
    _stores.clear()
    return total


def _get_or_create(name: str, cache: bool = True) -> VectorStore:
    if cache and name in _stores:
        return _stores[name]
    store = ChromaVectorStore(settings.vector_store_dir, name)
    if cache:
        _stores[name] = store
    return store
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `cd ai-service && python -m pytest tests/test_vector_store_multi.py -v`
Expected: ALL PASS

- [ ] **Step 5: Commit**

```bash
git add ai-service/app/services/vector_store.py ai-service/tests/test_vector_store_multi.py
git commit -m "feat: VectorStore 多 collection 工厂 + clear_all()"
```

---

### Task 2: Schema — add knowledge_type to request models

**Files:**
- Modify: `ai-service/app/schemas/ai.py`

**Interfaces:**
- Produces: `IndexRequest.knowledge_type: str`
- Produces: `IndexRemoveRequest.knowledge_type: str`
- Produces: `QaRequest.knowledge_type: str`

- [ ] **Step 1: Add `knowledge_type` to three request models**

```python
class IndexRequest(BaseModel):
    article_id: int = Field(..., description="所属知识 ID")
    knowledge_type: str = Field(..., description="SCRIPT/TRAIN/PRODUCT/OFFICE")
    file_path: str | None = None
    texts: list[str] | None = None


class IndexRemoveRequest(BaseModel):
    article_id: int
    knowledge_type: str = Field(..., description="SCRIPT/TRAIN/PRODUCT/OFFICE")


class QaRequest(BaseModel):
    question: str
    knowledge_type: str = Field(..., description="SCRIPT/TRAIN/PRODUCT/OFFICE")
    top_k: int = Field(5, description="召回条数")
    allowed_article_ids: list[int] | None = Field(
        None, description="允许命中的知识 ID（None=不限）"
    )
```

- [ ] **Step 2: Verify existing tests still pass (schemas only — no behavior change yet)**

Run: `cd ai-service && python -c "from app.schemas.ai import IndexRequest; r=IndexRequest(article_id=1, knowledge_type='SCRIPT'); print(r)"`
Expected: No import error, prints model

- [ ] **Step 3: Commit**

```bash
git add ai-service/app/schemas/ai.py
git commit -m "feat(schema): IndexRequest/IndexRemoveRequest/QaRequest 增加 knowledge_type"
```

---

### Task 3: QA — add knowledge_type routing

**Files:**
- Modify: `ai-service/app/services/qa.py`

**Interfaces:**
- Consumes: `get_vector_store(knowledge_type)` from Task 1
- Consumes: `QaRequest.knowledge_type` from Task 2
- Produces: `answer(question, knowledge_type="SCRIPT", top_k=5, allowed_article_ids=None) -> dict`

- [ ] **Step 1: Write the test update**

Append to `ai-service/tests/test_vector_store_multi.py`:

```python
from app.services import qa
from app.services.vector_store import get_vector_store


def test_qa_routes_to_correct_collection():
    import numpy as np
    from app.services import embedding

    # Seed SCRIPT collection with refund text
    store = get_vector_store("SCRIPT")
    store.clear_all()
    text = "退款政策：七天无理由退款，审核通过后 1-3 个工作日到账。"
    vec, _ = embedding.embed_texts([text])
    store.upsert(90001, [text], vec)

    # Query with knowledge_type="SCRIPT" should hit
    res = qa.answer("退款多久到账？", knowledge_type="SCRIPT", top_k=3)
    assert res["mode"] != "no_hit", f"SCRIPT 库应有命中: {res}"
    assert any(c["article_id"] == 90001 for c in res["citations"])

    # Query with knowledge_type="TRAIN" should no_hit (different collection)
    res_empty = qa.answer("退款多久到账？", knowledge_type="TRAIN", top_k=3)
    assert res_empty["mode"] == "no_hit", "TRAIN 库不应有退款数据"
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd ai-service && python -m pytest tests/test_vector_store_multi.py::test_qa_routes_to_correct_collection -v`
Expected: TypeError — `answer() got unexpected keyword argument 'knowledge_type'`

- [ ] **Step 3: Add `knowledge_type` parameter to `answer()`**

Change the function signature in `qa.py` (line ~107):

```python
def answer(
    question: str,
    knowledge_type: str = "SCRIPT",
    top_k: int = 5,
    allowed_article_ids: list[int] | None = None,
) -> dict:
```

And change the query call (line ~124):

```python
    store = get_vector_store(knowledge_type)
    pool = store.query(qvec, top_k=pool_k, allowed_article_ids=allowed_article_ids)
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `cd ai-service && python -m pytest tests/test_vector_store_multi.py -v`
Expected: ALL PASS

- [ ] **Step 5: Commit**

```bash
git add ai-service/app/services/qa.py ai-service/tests/test_vector_store_multi.py
git commit -m "feat(qa): answer() 增加 knowledge_type 参数，按类型路由检索"
```

---

### Task 4: API endpoints — wire knowledge_type through

**Files:**
- Modify: `ai-service/app/api/ai.py`
- Modify: `ai-service/app/main.py`

**Interfaces:**
- Consumes: `IndexRequest.knowledge_type`, `IndexRemoveRequest.knowledge_type`, `QaRequest.knowledge_type`
- Consumes: `clear_all()` from Task 1
- Produces: Updated `/ai/index`, `/ai/index/remove`, `/ai/qa` endpoints

- [ ] **Step 1: Update `/ai/index` endpoint** (line ~78–91 in `ai.py`)

Change the `index` function to pass `knowledge_type`:

```python
@router.post("/index", response_model=IndexResponse)
def index(req: IndexRequest) -> IndexResponse:
    if req.file_path:
        chunks = _parse_to_chunks(req.file_path)
    elif req.texts:
        chunks = [t for t in (s.strip() for s in req.texts) if t]
    else:
        raise HTTPException(status_code=status.HTTP_400_BAD_REQUEST,
                            detail="file_path 与 texts 至少提供其一")

    vectors, dim = embedding.embed_texts(chunks)
    store = get_vector_store(req.knowledge_type)
    count = store.upsert(req.article_id, chunks, vectors)
    return IndexResponse(article_id=req.article_id, chunk_count=count, dim=dim)
```

- [ ] **Step 2: Update `/ai/index/remove` endpoint** (line ~94–98 in `ai.py`)

```python
@router.post("/index/remove")
def index_remove(req: IndexRemoveRequest) -> dict:
    removed = get_vector_store(req.knowledge_type).remove_article(req.article_id)
    return {"article_id": req.article_id, "removed": removed}
```

- [ ] **Step 3: Update `/ai/qa` endpoint** (line ~101–114 in `ai.py`)

```python
@router.post("/qa", response_model=QaResponse)
def qa_endpoint(req: QaRequest) -> QaResponse:
    result = qa.answer(
        req.question,
        knowledge_type=req.knowledge_type,
        top_k=req.top_k,
        allowed_article_ids=req.allowed_article_ids,
    )
    return QaResponse(
        answer=result["answer"],
        citations=[Citation(**c) for c in result["citations"]],
        mode=result["mode"],
    )
```

- [ ] **Step 4: Update startup to use `clear_all()`** in `main.py`

Change the lifespan function (line ~28–31):

```python
    if settings.rebuild_on_startup:
        from app.services.vector_store import clear_all
        removed = clear_all()
        logger.info("启动自清：已清空 %d 条向量（Java 启动后将全量重建）", removed)
```

- [ ] **Step 5: Remove unused direct imports**

In `main.py`, remove the unused `from app.services.vector_store import get_vector_store` import, since we now use `clear_all` instead. The line is:

```python
from app.services.vector_store import get_vector_store
```

Replace with nothing (or keep if still needed elsewhere — currently it's only used in the lifespan block which now uses `clear_all`).

- [ ] **Step 6: Verify endpoints work**

Run: `cd ai-service && python -c "
from app.schemas.ai import IndexRequest, IndexRemoveRequest, QaRequest
r1 = IndexRequest(article_id=1, knowledge_type='SCRIPT')
r2 = IndexRemoveRequest(article_id=1, knowledge_type='SCRIPT')
r3 = QaRequest(question='test', knowledge_type='SCRIPT')
print('Schema OK:', r1, r2, r3)
"`
Expected: No errors

- [ ] **Step 7: Commit**

```bash
git add ai-service/app/api/ai.py ai-service/app/main.py
git commit -m "feat(api): endpoints 透传 knowledge_type，启动重建用 clear_all()"
```

---

### Task 5: Update validate scripts for backward compat

**Files:**
- Modify: `ai-service/scripts/validate_qa.py`

**Notes:** `validate_index.py` already uses `get_vector_store()` (no args) and calls `store.upsert`/`store.query` directly — it doesn't go through `qa.answer()`. So it works unchanged.

`validate_qa.py` calls `qa.answer(q, top_k=3)` — since we added `knowledge_type="SCRIPT"` as default, this also works unchanged. But we should update it to explicitly pass a knowledge_type to demonstrate the new API.

- [ ] **Step 1: Update `validate_qa.py` to pass `knowledge_type`**

Change line ~76 (normal query) and line ~84 (scoped query):

```python
    res = qa.answer(q, knowledge_type="SCRIPT", top_k=3)
    ...
    scoped = qa.answer(q, knowledge_type="SCRIPT", top_k=3, allowed_article_ids=[])
```

- [ ] **Step 2: Verify validate script still works**

Run: `cd ai-service && python scripts/validate_qa.py`
Expected: prints `[OK]` summary, exit code 0

- [ ] **Step 3: Commit**

```bash
git add ai-service/scripts/validate_qa.py
git commit -m "chore: validate_qa.py 显式传 knowledge_type"
```

---

### Task 6: Config — optional cleanup (low priority)

**Files:**
- Modify: `ai-service/app/core/config.py`

- [ ] **Step 1: Add a comment noting `collection_name` is only used by `get_vector_store(None)`**

Add after line 34 (`collection_name: str = ...`):

```python
    # 注：多 collection 分库后此默认 collection 仅用于向后兼容（get_vector_store(None)）。
    # 新代码通过 get_vector_store(knowledge_type) 获取对应类型的独立 collection。
```

- [ ] **Step 2: Commit**

```bash
git add ai-service/app/core/config.py
git commit -m "chore: 标注 collection_name 向后兼容用途"
```

---

### Task 7: Java 编排层适配指南

**Files:**
- Modify: Java 项目 `server/` 中的 AI client 类

**Context:** This task is in a separate codebase (Java/Spring Boot). Implementation is done by the Java team.

**Changes needed:**

1. **AI client request DTOs** — Add `knowledgeType` field to request objects

```java
// AiIndexRequest.java
@Data
public class AiIndexRequest {
    private Long articleId;
    private String knowledgeType;  // SCRIPT/TRAIN/PRODUCT/OFFICE
    private String filePath;
    private List<String> texts;
}

// AiIndexRemoveRequest.java
@Data
public class AiIndexRemoveRequest {
    private Long articleId;
    private String knowledgeType;
}

// QaRequest.java
@Data
public class QaRequest {
    private String question;
    private String knowledgeType;  // SCRIPT/TRAIN/PRODUCT/OFFICE
    private Integer topK;
    private List<Long> allowedArticleIds;
}
```

2. **Caller sites** — Pass `knowledgeType` when invoking AI service API:

| 场景 | Java 调用点 | 值来源 |
|---|---|---|
| 知识上线 → 向量化入库 | `AiServiceClient.index()` | `kb_article.knowledgeType` |
| 知识下线/删除 → 移除向量 | `AiServiceClient.removeIndex()` | `kb_article.knowledgeType` |
| 用户问答 | `AiServiceClient.qa()` | 前端选择的知识类型 |

- [ ] *No code steps in this repo — inform Java team of contract changes*
