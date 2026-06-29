# 多知识类型分库设计文档

> 基于 Chroma 向量库，将当前单 collection `kb_knowledge` 按 `knowledge_type` 拆分为 4 个独立 collection，实现知识分类隔离检索。

---

## 1. 背景

当前 ai-service 使用 Chroma 单 collection `kb_knowledge` 存储所有知识的向量块，无知识类型区分。问答时通过 `allowed_article_ids` 做权限/范围过滤，但无法按知识类型（话术/培训/产品/办公文档）限定检索范围。

需求文档 §10.8 明确划分了四大知识库：

| 知识库 | knowledge_type | 说明 |
|---|---|---|
| 客服话术库 | `SCRIPT` | 标准应答话术、安抚话术 |
| 培训学习库 | `TRAIN` | 培训材料、学习文档 |
| 产品知识库 | `PRODUCT` | 产品规格、功能介绍 |
| 办公文档库 | `OFFICE` | 制度、流程、办公文档 |

## 2. 设计目标

- 每个 `knowledge_type` 有独立的 Chroma collection
- 检索/问答时精确限定类型，不跨库
- 知识上线/下线实时同步到对应 collection
- 改动最小化，不改变 VectorStore 接口抽象

## 3. Collection 命名与生命周期

### 3.1 命名规则

```
kb_knowledge_{type_lower}
```

| knowledge_type | Collection 名 |
|---|---|
| `SCRIPT` | `kb_knowledge_script` |
| `TRAIN` | `kb_knowledge_train` |
| `PRODUCT` | `kb_knowledge_product` |
| `OFFICE` | `kb_knowledge_office` |

### 3.2 生命周期

- **首次写入**：Chroma `get_or_create_collection` 自动创建，无需预初始化
- **启动重建**（`KB_AI_REBUILD_ON_STARTUP=true`）：遍历全部 4 个 collection 逐一清空重建
- **类型扩展**：新增类型只需在 `_KNOWLEDGE_TYPES` 元组中追加，无需改其他代码

## 4. VectorStore 改造

### 4.1 多 Collection 工厂

```python
_KNOWLEDGE_TYPES = ("SCRIPT", "TRAIN", "PRODUCT", "OFFICE")
_stores: dict[str, VectorStore] = {}

def _collection_name(knowledge_type: str) -> str:
    return f"kb_knowledge_{knowledge_type.lower()}"

def get_vector_store(knowledge_type: str | None = None) -> VectorStore:
    """获取对应知识类型的向量库。
    
    Args:
        knowledge_type: SCRIPT/TRAIN/PRODUCT/OFFICE 之一。
                        None 返回旧版单 collection（向后兼容/启动重建用）。
    """
    if knowledge_type is None:
        return _get_or_create("kb_knowledge")
    return _get_or_create(_collection_name(knowledge_type))
```

### 4.2 批量清空

增加 `clear_all()` 函数，遍历所有已知类型 + 已缓存的 collection 逐一清空：

```python
def clear_all() -> int:
    total = 0
    # 清空所有已知类型的 collection（即使未缓存）
    for kt in _KNOWLEDGE_TYPES:
        store = _get_or_create(_collection_name(kt), cache=False)
        total += store.clear_all()
    # 清空默认 collection（旧版入口，如有残留）
    default = _get_or_create("kb_knowledge", cache=False)
    total += default.clear_all()
    # 清除缓存引用
    _stores.clear()
    return total
```

### 4.3 ChromaVectorStore 不变

`ChromaVectorStore` 已经是按 `collection_name` 构造的，无需修改方法体。

## 5. API 契约变更

### 5.1 POST /ai/index

新增 `knowledge_type` 必填字段，路由到对应 collection 入库。

```python
class IndexRequest(BaseModel):
    article_id: int
    knowledge_type: str = Field(..., description="SCRIPT/TRAIN/PRODUCT/OFFICE")
    file_path: str | None = None
    texts: list[str] | None = None
```

### 5.2 POST /ai/index/remove

新增 `knowledge_type` 必填字段。

```python
class IndexRemoveRequest(BaseModel):
    article_id: int
    knowledge_type: str = Field(..., description="SCRIPT/TRAIN/PRODUCT/OFFICE")
```

### 5.3 POST /ai/qa

新增 `knowledge_type` 必填字段，检索限定在该 collection。

```python
class QaRequest(BaseModel):
    question: str
    knowledge_type: str = Field(..., description="SCRIPT/TRAIN/PRODUCT/OFFICE")
    top_k: int = 5
    allowed_article_ids: list[int] | None = None
```

## 6. QA 流程变更

`qa.answer()` 新增 `knowledge_type` 参数，检索时路由到对应 collection：

```python
def answer(question: str, knowledge_type: str, top_k: int = 5,
           allowed_article_ids: list[int] | None = None) -> dict:
    ...
    store = get_vector_store(knowledge_type)
    pool = store.query(qvec, top_k=pool_k, allowed_article_ids=allowed_article_ids)
    ...
```

精排、LLM 总结、抽取式兜底的后续流程不变。

## 7. 启动重建

`KB_AI_REBUILD_ON_STARTUP=true` 时调用 `clear_all()` 清空所有 collection：

```python
# app/main.py
from app.services.vector_store import clear_all

if settings.rebuild_on_startup:
    clear_all()
```

## 8. Java 编排层适配

Java 侧需做以下改动：

1. **知识上线** → 调 `/ai/index` 时，从 `kb_article.knowledge_type` 取值传入
2. **知识下线/删除** → 调 `/ai/index/remove` 时，传对应的 `knowledge_type`
3. **问答** → 前端选择的知识分类映射为 `knowledge_type` 传入

```java
// Java 编排示例
AiIndexRequest req = new AiIndexRequest();
req.setArticleId(article.getId());
req.setKnowledgeType(article.getKnowledgeType());  // "SCRIPT"
req.setFilePath(article.getFilePath());
aiClient.index(req);

QaRequest qaReq = new QaRequest();
qaReq.setQuestion(question);
qaReq.setKnowledgeType(knowledgeType);  // 前端选择
qaReq.setTopK(5);
qaReq.setAllowedArticleIds(permittedIds);
```

## 9. 改动文件清单

| 文件 | 改动类型 |
|---|---|
| `ai-service/app/schemas/ai.py` | IndexRequest/IndexRemoveRequest/QaRequest 增加 `knowledge_type` |
| `ai-service/app/services/vector_store.py` | 增加多 collection 工厂 `get_vector_store(type)` + `clear_all()` |
| `ai-service/app/services/qa.py` | `answer()` 增加 `knowledge_type` 参数，检索时路由 |
| `ai-service/app/api/ai.py` | 端点透传 `knowledge_type`；启动排除旧 collection 配置 |
| `ai-service/app/core/config.py` | 可选去掉 `collection_name` 单一配置 |
| Java 编排层 | 调用 AI API 时传入 `knowledge_type` |

## 10. 向后兼容

- `get_vector_store(None)` 返回旧版单 collection `kb_knowledge`，旧版调用不中断
- 重建脚本 `validate_index.py` 等若未传 `knowledge_type`，仍可操作默认 collection
- 建议所有新代码显式传 `knowledge_type`，逐步迁移

---

*设计版本：v1.0*
*日期：2026-06-29*
