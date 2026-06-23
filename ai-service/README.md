# ai-service —— 智能客服知识库 AI 微服务（模块 9 智能问答）

Python 3.12 + FastAPI。**M9.1–M9.4 全部落地**：附件解析 / 切分 / 本地 bge-zh 向量化 / Chroma 向量库 / RAG 问答（ms-agent 驱动 OpenAI 兼容 LLM）+ **LLM 运行时配置下发**（Java 推送→即时生效+落盘，无需重启）。未配 LLM 时抽取式兜底，无密钥也能端到端跑通。环境用 **conda** 管理（`kb-ai`）。

技术栈锁定：Chroma（进程内嵌、落盘）+ 本地 bge-small-zh-v1.5（CPU、512 维）+ 本地 bge-reranker-base 精排（CPU，可关）+ ms-agent 驱动第三方 OpenAI 兼容 LLM。

## 目录

```
ai-service/
├── environment.yml      # conda 环境（name: kb-ai, python 3.12）
├── requirements.txt     # pip 依赖（备选）
├── scripts/
│   ├── validate_index.py   # M9.1 端到端验证（解析→切分→embed→写库→检索），无需 Java/LLM
│   └── validate_qa.py      # M9.2 端到端验证（索引→问答），默认抽取式兜底、无需 Java/LLM
├── models/              # 本地模型（bge-small-zh-v1.5 向量 + bge-reranker-base 精排，gitignore）
├── vector_store/        # Chroma 落盘目录（gitignore，可由源文档重建）
├── runtime_llm_config.json  # LLM 运行时配置落盘（gitignore，Java 下发后持久化，启动 overlay）
└── app/
    ├── main.py          # FastAPI 应用 + 路由装配
    ├── api/             # health（可用）、ai（parse/embed/index/index.remove/qa 均已落地）
    ├── core/            # config（KB_AI_* 环境变量）/ logging
    ├── schemas/         # pydantic 模型 = 与 Java 的接口契约
    └── services/        # parser / chunker / embedding / reranker / vector_store / llm / qa（均已实现）
```

## 环境搭建

```bash
cd ai-service
conda env create -f environment.yml      # 建 kb-ai（torch/transformers/chromadb 等）
conda activate kb-ai
```

下载本地 embedding 模型（经魔搭，约 95MB）到 `models/bge-small-zh-v1.5`：

```bash
python -c "from modelscope import snapshot_download; snapshot_download('AI-ModelScope/bge-small-zh-v1.5', local_dir='models/bge-small-zh-v1.5')"
```

> 若 `models/bge-small-zh-v1.5` 不存在，embedding 层会回退为按模型名 `BAAI/bge-small-zh-v1.5` 联网加载。

精排模型 bge-reranker-base（约 1.1GB）**首次问答时自动下载**到 `models/bge-reranker-base`；也可预下载：

```bash
python -c "from modelscope import snapshot_download; snapshot_download('BAAI/bge-reranker-base', local_dir='models/bge-reranker-base')"
```

> 不想用精排（或想省去该模型）设 `KB_AI_RERANK_ENABLED=false` 即可，问答退回纯向量检索。Apple Silicon 可设 `KB_AI_RERANK_DEVICE=mps` 给精排提速。

## 运行

```bash
cd ai-service
conda activate kb-ai
uvicorn app.main:app --reload            # http://127.0.0.1:8000/health ；交互文档 /docs
```

## 验证（无需 Java / 无需 LLM 令牌）

```bash
conda run -n kb-ai python scripts/validate_index.py
# 造样例 → 解析→切分→embed→写 Chroma→检索；断言 dim=512、命中≥1、范围过滤生效、向量库落盘

conda run -n kb-ai python scripts/validate_qa.py
# 造样例 → 索引 → 问答；断言 mode∈{llm,extractive}、Top1=退款篇、snippet 存在、空范围→no_hit
# 未配 KB_AI_LLM_* 时走抽取式兜底（mode=extractive），零令牌消耗
```

## 配置项（环境变量，KB_AI_* 前缀，见 app/core/config.py）

| 变量 | 默认 | 说明 |
|---|---|---|
| `KB_AI_STORAGE_DIR` | `<repo>/uploads` | 附件存储根；相对 `file_path` 在此解析（对齐 Java LocalFileStorage） |
| `KB_AI_VECTOR_STORE_DIR` | `ai-service/vector_store` | Chroma 落盘目录 |
| `KB_AI_EMBEDDING_MODEL` | `ai-service/models/bge-small-zh-v1.5` | 本地模型目录；缺失则联网回退 |
| `KB_AI_COLLECTION` | `kb_knowledge` | Chroma collection 名 |
| `KB_AI_CHUNK_SIZE` / `KB_AI_CHUNK_OVERLAP` | `500` / `80` | 切分长度 / 重叠（按字符） |
| `KB_AI_LLM_BASE_URL` / `KB_AI_LLM_API_KEY` / `KB_AI_LLM_MODEL` | 空 | 第三方 OpenAI 兼容 LLM（三者齐备才接入；未配则抽取式兜底） |
| `KB_AI_LLM_TEMPERATURE` / `KB_AI_LLM_MAX_TOKENS` | `0.2` / `1024` | LLM 生成参数（经 ms-agent 按签名透传） |
| `KB_AI_QA_MIN_SCORE` | `0.3` | 问答相关性下限；低于此分的命中丢弃（用于「无相关知识」短路）。**仅在精排关闭时生效** |
| `KB_AI_RETRIEVE_K` | `30` | 向量宽召回候选数（送精排）；越大召回越全、精排越慢 |
| `KB_AI_MAX_PER_ARTICLE` | `3` | 单篇知识最多入 LLM 上下文的块数（兼顾跨篇覆盖与单篇深度） |
| `KB_AI_RERANK_ENABLED` | `true` | 是否启用 bge-reranker 交叉编码器精排；关闭则纯向量分排序 |
| `KB_AI_RERANK_MODEL` | `ai-service/models/bge-reranker-base` | 本地精排模型目录；缺失自动从 ModelScope 下载、失败回退 HF |
| `KB_AI_RERANK_DEVICE` | `cpu` | 精排推理设备（Apple Silicon 可设 `mps` 提速） |
| `KB_AI_RERANK_MIN_SCORE` | `0.3` | 精排相关性下限（sigmoid 归一 0~1）；**启用精排时取代 `KB_AI_QA_MIN_SCORE`** 做「无相关知识」短路 |
| `KB_AI_RUNTIME_LLM_CONFIG` | `ai-service/runtime_llm_config.json` | LLM 运行时配置落盘路径（Java 下发后持久化，启动时 overlay 覆盖 env 默认值） |

## Java ↔ FastAPI 契约

| 接口 | 入参 | 出参 | 触发点 | 状态 |
|---|---|---|---|---|
| `POST /ai/parse` | article_id / file_id / file_path | 文本块 chunks | 调试/解析切分 | ✅ M9.1 |
| `POST /ai/embed` | texts | vectors + dim | 取原始向量（不写库） | ✅ M9.1 |
| `POST /ai/index` | article_id + file_path 或 texts | article_id + chunk_count + dim | 知识上线 → 向量化并入库 | ✅ M9.1 |
| `POST /ai/index/remove` | article_id | removed | 知识下线 → 删除向量块 | ✅ M9.1 |
| `POST /ai/qa` | question + top_k + allowed_article_ids | answer + citations + mode | 用户提问（权限/上线过滤由 Java 完成） | ✅ M9.2 |
| `GET /ai/llm/config` | — | configured + base_url + model + temperature + max_tokens（**无 api_key**） | Java 查询 LLM 配置状态 | ✅ M9.4 |
| `POST /ai/llm/config` | base_url + api_key + model + temperature + max_tokens | 同上 | Java 推送 LLM 配置→即时生效+落盘 | ✅ M9.4 |
| `POST /ai/llm/test` | base_url + api_key + model + temperature + max_tokens | ok + message + latency_ms | Java 测试 LLM 连接（临时客户端，不改单例） | ✅ M9.4 |

入参/出参字段定义见 `app/schemas/ai.py`（pydantic 模型即契约）。向量库写入/检索由本服务负责；权限与上线状态过滤由 Java 编排层完成（传 `allowed_article_ids`）。

## 智能问答（`/ai/qa`，M9.2）

检索增强问答（RAG），两段式检索：`embed_query` 向量化问题 → Chroma **宽召回 `KB_AI_RETRIEVE_K` 条**（双塔，快但粗）→ **bge-reranker 交叉编码器精排**（拼 (问题, 片段) 直接打分，精度高）→ 按 `KB_AI_RERANK_MIN_SCORE`（精排关闭时为 `KB_AI_QA_MIN_SCORE`）过滤 → 选 Top-`top_k` 片段（**同篇可多块、单篇封顶 `KB_AI_MAX_PER_ARTICLE`**）作答。精排禁用或模型加载失败时自动退回纯向量分排序，问答不受影响。答案来源由出参 `mode` 标识：

- `llm`：配齐 `KB_AI_LLM_*` 三件套时，经 **ms-agent 框架**（`service=openai`，底层即 OpenAI 兼容 `base_url`）单轮总结；system 提示约束「仅依据参考资料作答、资料不足就说没有并建议转人工」防臆造。
- `extractive`：未配 LLM 或调用失败/空答时的**抽取式兜底**，直接返回高分原文片段；无密钥也能端到端跑通。
- `no_hit`：空问题或过滤后无相关命中，返回友好提示。

`citations` 与送 LLM 的上下文同源，按 `article_id` 去重（同篇取最相关块）、按相关分降序，含 `snippet`（命中原文片段，供前端核对）；`score` 在启用精排时为精排分（sigmoid 归一 0~1），否则为余弦相似度。`title` 在本服务为占位 `知识 #{id}`，真实标题由 Java 编排层（M9.3）用 `kb_article.title` 回填。ms-agent 的工具/MCP/多步能力留给后续模块 10。

## LLM 运行时配置（`/ai/llm/config` + `/ai/llm/test`，M9.4）

管理员在 Qt 客户端「AI 配置」页填写/测试/保存 LLM 接入配置，Java 存 DB（权威源）后推送到以下端点：

- **`POST /ai/llm/config`**：接收 Java 下发的 `{base_url, api_key, model, temperature, max_tokens}` → `llm.apply_config()` 改 `settings.llm_*` + `llm.reset()` 重建单例 + 写 `runtime_llm_config.json` 落盘。若 `base_url`/`api_key`/`model` 任一为空字符串，则 `is_configured()` 为 false，问答回退抽取式。
- **`GET /ai/llm/config`**：返回当前配置状态（`configured` + `base_url`/`model`/`temperature`/`max_tokens`），**不含 `api_key`**。
- **`POST /ai/llm/test`**：用请求体中的候选配置建**临时** LLM 客户端做一次极短 `generate`（"回复两个字：你好"），返回 `{ok, message, latency_ms}`。**不落库、不修改运行中单例**。
- **启动恢复**：`app/core/config.py` 在模块加载末尾调用 `_overlay_runtime_llm()`，若 `runtime_llm_config.json` 存在则覆盖 env 默认值 → Python 自身重启不丢配置。Java 侧 `LlmConfigSyncRunner`@`ApplicationReadyEvent` 在 Java 启动时补发库中已启用配置。

> `runtime_llm_config.json` 含明文 `api_key`，已进 `.gitignore`。部署环境建议配置文件系统权限限制。
