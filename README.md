# 坐席智能体 · 智能客服知识库系统

面向客服场景的知识管理与应用平台：统一存储话术、培训、产品与办公文档，提供检索、互动、统计与开放 API；二期接入 **RAG 智能问答**、**首页个性化推荐** 与 **坐席实时辅助**（Mock ASR + WebSocket 推送）。

桌面客户端品牌：**坐席智能体**；侧栏字标：**硅基生命坐席**（耳机 + 机器人 Logo + PNG 艺术字）。

---

## 架构

```
Qt Widgets 桌面客户端 (knowledgeAnswer/)
        │  REST/JSON + WebSocket（实时辅助）
        ▼
Java / Spring Boot 业务后端 (server/)     ──REST──▶  Python / FastAPI AI 服务 (ai-service/)
        │
        ├── MySQL 8（业务数据、行为日志、RBAC、QA 会话）
        ├── 本地文件存储（uploads/，附件抽象可换 MinIO）
        └── Redis（可选，文档规划用于缓存/限流，当前未接入）
```

| 层级 | 技术栈 | 职责 |
|------|--------|------|
| **客户端** | Qt 6.11 · C++17 · CMake · QSS（花笺暖色主题） | 登录、导航、知识门户、管理后台、问答、坐席辅助面板 |
| **业务后端** | Java 17 · Spring Boot 3.3 · Security + JWT · MyBatis-Plus · Flyway | 鉴权 RBAC、知识 CRUD/审核、检索、互动、统计、开放 API、AI/实时编排 |
| **AI 服务** | Python 3.12 · FastAPI · Chroma · bge-small-zh-v1.5 · ms-agent | 解析/切分/向量化、Chroma 检索、RAG 问答、推荐向量匹配、LLM 运行时配置 |

**调用约束**：Qt 客户端 **不直连** Python；LLM Key 等敏感配置存 MySQL，由 Java 以 `runtime_config` 下发给 AI 服务，Python 侧无持久化密钥。

---

## 功能概览

### 一期 · 知识中台（模块 1–8）

| 模块 | 能力 | 客户端页面 / 后端入口 |
|------|------|------------------------|
| **1 登录鉴权** | JWT 登录、自助注册、RBAC 菜单/按钮权限 | `LoginDialog` · `/auth/*` |
| **2 系统管理** | 机构树、用户/角色、菜单权限、登录/操作日志 | `SystemAdminPage` · `/system/*` |
| **3 知识基础** | 分类树（级联删除）、标签 CRUD | `KnowledgeBasePage` · `/knowledge/category` `/knowledge/tag` |
| **4 知识管理** | 文章 CRUD、版本、审核流、上下线、附件上传下载、标签挂接 | `ArticleManagePage` · `/knowledge/article` |
| **5 检索与详情** | MySQL 全文检索（ngram）、分类/标签/作者筛选、offset 信息流、浏览埋点 | `SearchPage` + 详情弹窗 · `/search/article` `/knowledge/article/{id}/view` |
| **6 知识互动** | 收藏、点赞/点踩、评论树、分享给同事、站内通知 | 详情互动条 · `/interaction/*`；`SharePage` |
| **7 数据统计** | 平台概览、热门知识/搜索词、趋势、分类排行、审核/问答运营 | `StatisticsPage` · `/statistics/*` |
| **8 开放 API** | 开放应用管理；对外检索/详情/问答（AppKey + HMAC + scope + 限流） | `OpenApiPage` · `/openapi/app` · `/open/v1/*` |

**知识社区扩展**（Flyway V12–V14）：个人中心（发布/收藏/隐私）、社区发文与草稿编辑、用户主页与 Feed 流。

### 二期 · 智能与实时（模块 9–10）

| 模块 | 能力 | 说明 |
|------|------|------|
| **9 智能问答** | 向量索引 + RAG + 会话历史 + 反馈 | `QaPage`；Java `QaService` → Python `/ai/qa`；未配 LLM 时抽取式兜底 |
| **9+ AI 配置** | 管理员配置 LLM Base/Key/Model、测试连接、重建索引 | `AiConfigPage`（route `ai.config`）；`/ai/llm-config` → 下发 Python |
| **9+ 首页推荐** | 行为画像 + bge 向量匹配 + 热度加权 Top5 | `DashboardPage`「为你推荐」；`/knowledge/recommend/home` |
| **10 坐席实时辅助** | Mock ASR 转写、WebSocket 推送、定时 RAG 推荐 Top3 | `AgentAssistPage`；`/realtime/*` + `/ws/realtime/{sessionId}` |

### 客户端信息架构

```
顶栏 #TopBar（浅米黄 #FAF6F0）
  左侧：当前页图标 + PageTitle（随导航切换）
  右侧：刷新 · 角色徽章 · 用户 · 退出

侧栏 #NavSidebar（#F3EDE4）
  品牌区：Logo + 「硅基生命坐席」PNG 字标
  导航树：按 /auth/me 返回的菜单动态构建（RBAC）

主内容：PageRouter 懒加载各业务页
```

主要页面：`DashboardPage` · `SearchPage` · `QaPage` · `ArticleManagePage` · `AuditCenterPage` · `KnowledgeBasePage` · `StatisticsPage` · `OpenApiPage` · `AiConfigPage` · `AgentAssistPage` · `ProfilePage` · `SharePage` · `SystemAdminPage`

---

## 仓库结构

```
tuandui/
├── README.md
├── CLAUDE.md                 # AI 辅助开发说明（架构/命令/约定）
├── docs/
│   ├── 智能客服知识库系统需求分析与开发计划_Qt.md
│   ├── 总体开发计划.md
│   ├── 开发进度.md           # 活文档
│   ├── API契约.md
│   ├── 数据库表清单.md
│   ├── Qt前端视觉风格规范.md
│   ├── 首页个性化推荐算法说明.md
│   └── superpowers/          # 模块设计/实施计划归档
├── knowledgeAnswer/          # Qt 桌面客户端
│   ├── app/                  # 页面、主窗口、登录
│   ├── core/                 # ApiClient、Session、Settings
│   ├── common/               # 导航委托、主题图标
│   └── resources/            # app.qss、icons、images
├── server/                   # Java Spring Boot
│   └── src/main/
│       ├── java/com/kb/      # 按域分包：system/knowledge/interaction/statistics/openapi/ai/realtime/recommend…
│       └── resources/db/migration/   # Flyway V1–V15
└── ai-service/               # Python FastAPI
    ├── app/                  # api / services / schemas
    ├── scripts/              # validate_index.py、validate_qa.py
    ├── models/               # bge 模型（gitignore）
    └── vector_store/         # Chroma 持久化（gitignore）
```

---

## 环境要求

| 部分 | 依赖 |
|------|------|
| **客户端** | Qt 6.11（Widgets / Network / WebSockets / TextToSpeech / PDF / Multimedia）、CMake ≥ 3.16、MinGW 64-bit |
| **后端** | JDK 17、Maven 3.9+、MySQL 8 |
| **AI 服务** | conda、Python 3.12；环境名 `kb-ai`（见 `ai-service/environment.yml`） |
| Redis（缓存/分布式限流，规划中） |

---

## 快速启动

### 1. 数据库

```bash
# 以 root 创建库与专用账号（仅需一次）
mysql -u root -p -e "
  CREATE DATABASE IF NOT EXISTS kb CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
  CREATE USER IF NOT EXISTS 'kb'@'localhost' IDENTIFIED BY '123456';
  GRANT ALL ON kb.* TO 'kb'@'localhost';
  FLUSH PRIVILEGES;
"
```

后端首启时 **Flyway 自动迁移**（当前 V1–V15）。本地口令覆盖可写 `server/src/main/resources/application-local.yml`（已 gitignore）。

### 2. 业务后端

```bash
cd server && mvn spring-boot:run
# 健康检查：http://localhost:8080/api/actuator/health
# Swagger：   http://localhost:8080/api/swagger-ui.html
```

### 3. AI 服务

```bash
cd ai-service
conda env create -f environment.yml   # 首次
conda activate kb-ai

# 下载 embedding 模型（约 95MB，首次）
python -c "from modelscope import snapshot_download; snapshot_download('AI-ModelScope/bge-small-zh-v1.5', local_dir='models/bge-small-zh-v1.5')"

uvicorn app.main:app --host 127.0.0.1 --port 8000
# 文档：http://127.0.0.1:8000/docs
# 启动时会 warmup embedding（KB_AI_WARMUP_ON_STARTUP，默认 true）
```

独立冒烟（无需 Java）：

```bash
python scripts/validate_index.py
python scripts/validate_qa.py
```

### 4. Qt 客户端

```bash
cmake -S knowledgeAnswer -B knowledgeAnswer/build
cmake --build knowledgeAnswer/build
# 或在 Qt Creator 打开 knowledgeAnswer/CMakeLists.txt，选择 MinGW Kit 构建运行
```

**推荐启动顺序**：MySQL → Java 后端 → AI 服务 → Qt 客户端 → 使用 `admin / 123456` 登录。

### 演示数据（可选）

```bash
mysql -u kb -p123456 --default-character-set=utf8mb4 kb < server/src/main/resources/db/demo/demo_data.sql
mysql -u kb -p123456 --default-character-set=utf8mb4 kb < server/src/main/resources/db/demo/demo_data_community.sql
mysql -u kb -p123456 --default-character-set=utf8mb4 kb < server/src/main/resources/db/demo/demo_data_batch2.sql
```

| 账号 | 角色 | 密码 |
|------|------|------|
| `admin` | 管理员 | `123456` |
| `k_admin` | 知识管理员 | `123456` |
| `auditor1` | 审核员 | `123456` |
| `agent1` | 坐席 | `123456` |
| `user1` | 普通用户 | `123456` |

---

## 关键配置

| 文件 | 说明 |
|------|------|
| `server/src/main/resources/application.yml` | 端口 8080、MySQL `kb`、JWT、`kb.ai.base-url`、首页推荐权重 `kb.recommend.*` |
| `ai-service/app/core/config.py` | `KB_AI_VECTOR_STORE_DIR`、`KB_AI_EMBEDDING_MODEL`、`KB_AI_WARMUP_ON_STARTUP` 等 |
| `knowledgeAnswer/resources/styles/app.qss` | 花笺暖色主题（顶栏/侧栏/组件 objectName） |

AI 模型与 RAG 参数可在客户端 **AI 配置** 页（管理员）维护，保存至 MySQL 并下发 Python；**无需**在 `ai-service/` 目录单独配置 LLM Key。

---

## API 约定摘要

- **Base URL**：`http://<host>:8080/api`
- **统一响应**：`{ "code": 0, "message": "ok", "data": {} }`（`code=0` 成功）
- **鉴权**：`Authorization: Bearer <jwt>`（开放 API 用 AppKey + HMAC，见 `docs/API契约.md`）
- **错误码段**：0 成功 · 2xxx 鉴权 · 4xxx 知识 · 5xxx 检索/互动 · 6xxx 开放 API · 7xxx AI/QA

完整接口清单见 [`docs/API契约.md`](docs/API契约.md)。

---

## 智能问答与推荐（简图）

**RAG 问答**

```
用户提问 → Java QaService → Python /ai/qa
  → bge 向量化 → Chroma 检索 → 阈值过滤
  → LLM 已配置：ms-agent 总结 (mode=llm)
  → 未配置/失败：抽取式兜底 (mode=extractive)
```

**首页推荐**

```
DashboardPage → GET /knowledge/recommend/home
  → MySQL 行为（搜索/点赞）+ 客户端 pin 标签
  → Python POST /ai/recommend/match（bge 语义匹配）
  → 兴趣分 + 热度分 → Top5
```

详见 [`docs/首页个性化推荐算法说明.md`](docs/首页个性化推荐算法说明.md)。

---

## 开发阶段

| 阶段 | 范围 | 状态 |
|------|------|------|
| **一期** | 模块 1–8 知识中台 + 社区/个人中心扩展 | ✅ 已完成 |
| **二期 M9** | 智能问答、AI 配置、向量索引、首页推荐 | ✅ 已完成 |
| **二期 M10** | 坐席实时辅助（Mock ASR + WS + 实时推荐） | ✅ 已落地（CC/真实 ASR 对接为后续演进） |
| **优化项** | Redis 缓存层、ES 检索、TTS/PDF 预览、富文本编辑器等 | 📋 规划中 / 部分 stub |

活进度与变更记录：[`docs/开发进度.md`](docs/开发进度.md)

---

## 文档导航

| 文档 | 内容 |
|------|------|
| [`docs/智能客服知识库系统需求分析与开发计划_Qt.md`](docs/智能客服知识库系统需求分析与开发计划_Qt.md) | 需求与功能清单 |
| [`docs/总体开发计划.md`](docs/总体开发计划.md) | 架构、技术选型、编码顺序 |
| [`docs/开发进度.md`](docs/开发进度.md) | 当前任务进度（活文档） |
| [`docs/API契约.md`](docs/API契约.md) | REST 约定与接口草案 |
| [`docs/数据库表清单.md`](docs/数据库表清单.md) | 表结构与关键字段 |
| [`docs/Qt前端视觉风格规范.md`](docs/Qt前端视觉风格规范.md) | 色板、组件 objectName、品牌标识 |
| [`docs/首页个性化推荐算法说明.md`](docs/首页个性化推荐算法说明.md) | 推荐链路、权重、降级策略 |
| [`ai-service/README.md`](ai-service/README.md) | AI 微服务环境、验证、接口 |
| [`CLAUDE.md`](CLAUDE.md) | 仓库级开发指引（命令、目录、约定） |

---


