# 智能客服知识库系统

面向客服场景的知识管理与应用平台：统一存储/更新客服话术、培训、产品、办公文档等知识，提供检索、查看、朗读、收藏、评论、分享等应用能力，配套知识管理后台（录入/导入/审核/上下线/分类）、数据统计与知识能力开放 API。二期接入智能问答（向量检索 + 大模型）与客服实时辅助（CC 集成 / ASR 转写 / 实时推荐 / 坐席面板）。

## 架构

```
Qt Widgets 桌面客户端  ──REST/JSON(+WebSocket)──▶  Java / Spring Boot 业务后端  ──REST──▶  Python / FastAPI AI 服务（二期）
                                                          │
                                                  MySQL 8 / Redis / 本地文件存储
```

- **客户端** `knowledgeAnswer/`：Qt 6 + Qt Widgets + C++17 + CMake。
- **业务后端** `server/`：Java 17 + Spring Boot 3 + Spring Security + JWT + MyBatis-Plus + Flyway，包名 `com.kb`。
- **AI 服务** `ai-service/`：Python 3.11 + FastAPI（二期，先占位；**环境用 conda 管理**）。

## 仓库结构

```
thePro/
├── README.md
├── .gitignore
├── docs/
│   ├── 智能客服知识库系统需求分析与开发计划_Qt.md   # 需求文档
│   ├── 总体开发计划.md      # 架构 / 技术选型 / 目录 / 编码顺序
│   ├── 开发进度.md          # 活文档：当前任务进度
│   ├── 数据库表清单.md      # 一期建表范围与关键字段
│   ├── API契约.md           # REST 约定与接口草案
│   └── 需求图/              # 原始需求截图（归档）
├── knowledgeAnswer/         # Qt Widgets 桌面客户端
├── server/                  # Java Spring Boot 业务后端
└── ai-service/              # Python FastAPI AI 微服务（二期）
```

## 环境要求（Windows）

| 部分 | 依赖 |
|---|---|
| 客户端 | Qt 6.11（Widgets / Network / WebSockets / TextToSpeech / PDF / Multimedia）、CMake ≥ 3.16、MinGW 64-bit、Qt Creator |
| 后端 | JDK 17、Maven 3.9+、MySQL 8、Redis（可选） |
| AI 服务（二期） | conda、Python 3.11、FastAPI/uvicorn、向量库（pgvector/Milvus/Qdrant） |

## 构建与运行

> 当前处于「脚手架与空壳」阶段，以下命令在各端骨架落地后可用。

**客户端**

```bash
# Qt Creator 打开 knowledgeAnswer/CMakeLists.txt，选择 MinGW Kit，Configure → 构建运行
# 或命令行：
cmake -S knowledgeAnswer -B knowledgeAnswer/build && cmake --build knowledgeAnswer/build
```

**后端**

```bash
cd server && mvn spring-boot:run     # 首启由 Flyway 自动建表；Swagger: /swagger-ui.html
```

**演示 / 测试数据**（可选，独立脚本，非 Flyway 迁移；幂等可重跑）

```bash
# 后端首启建好表后执行；演示账号 k_admin/auditor1/agent1 等密码均 123456
mysql -u kb -p123456 --default-character-set=utf8mb4 kb \
  < server/src/main/resources/db/demo/demo_data.sql
```

**AI 服务（二期）**

```bash
conda env create -f ai-service/environment.yml && conda activate kb-ai
uvicorn app.main:app --reload        # /health
```

## 文档导航

- 需求：`docs/智能客服知识库系统需求分析与开发计划_Qt.md`
- 怎么做：`docs/总体开发计划.md`
- 进度：`docs/开发进度.md`
- 数据表 / 接口：`docs/数据库表清单.md`、`docs/API契约.md`

## 开发阶段

- **一期（知识中台核心）**：模块 1 登录鉴权+RBAC → 2 系统管理 → 3 知识基础 → 4 知识管理后台 → 5 检索+详情 → 6 知识互动 → 7 数据统计 → 8 开放 API。
- **二期（智能与实时）**：模块 9 智能问答、10 客服实时辅助。
