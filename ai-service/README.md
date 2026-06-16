# ai-service —— 智能客服知识库 AI 微服务（二期）

Python 3.11 + FastAPI。**当前为占位骨架**：仅 `/health` 可用；`/ai/*` 为契约占位（返回 501），具体能力在二期（模块 9 智能问答、模块 10 实时辅助）实现。环境用 **conda** 管理。

## 目录

```
ai-service/
├── environment.yml      # conda 环境（name: kb-ai, python 3.11）
├── requirements.txt     # pip 依赖（fastapi / uvicorn）
└── app/
    ├── main.py          # FastAPI 应用 + 路由装配 + /health 冒烟
    ├── api/             # health（可用）、ai（/ai/parse·/ai/embed·/ai/qa 占位 501）
    ├── core/            # config（环境变量）/ logging
    ├── schemas/         # pydantic 模型 = 与 Java 的接口契约
    └── services/        # 解析 / 切分 / Embedding / 向量库 / LLM（二期，空）
```

## 运行

conda（推荐）：

```bash
cd ai-service
conda env create -f environment.yml
conda activate kb-ai
uvicorn app.main:app --reload          # http://127.0.0.1:8000/health ；交互文档 /docs
```

pip（备选）：

```bash
cd ai-service
python -m venv .venv && source .venv/Scripts/activate   # Windows Git Bash
pip install -r requirements.txt
uvicorn app.main:app --reload
```

冒烟：`GET /health` → `{"status":"UP"}`；`/ai/*` 当前返回 `501 Not Implemented`。

## Java ↔ FastAPI 契约（二期）

| 接口 | 入参 | 出参 | 触发点 |
|---|---|---|---|
| `POST /ai/parse` | 附件引用（article_id / file_id / path） | 文本块 chunks | 知识上线 → 解析切分 |
| `POST /ai/embed` | 文本块 texts | 向量 vectors + dim | 切分后向量化入库 |
| `POST /ai/qa` | question + top_k + 允许范围 | answer + citations | 用户提问（权限/上线过滤由 Java 完成） |

入参/出参的字段定义见 `app/schemas/ai.py`（pydantic 模型即契约）。向量库的写入与检索由本服务负责。
