# 模块 10：客服实时辅助 — 设计文档

> MVP 版本设计。CC 集成 / ASR 转写用 Mock 模拟，重点实现 WebSocket 实时推送 + 坐席面板 UI + 实时推荐闭环。

---

## 1. 目标与范围

### 1.1 目标

为客服坐席提供实时辅助面板：通话过程中接收 ASR 转写文本，系统自动从知识库中匹配相关话术/知识并实时推送给坐席，辅助快速应答。

### 1.2 MVP 范围

| 功能 | 状态 |
|---|---|
| Mock ASR 模拟器 | ✅ MVP |
| 实时转写推送到坐席面板 | ✅ MVP |
| 定时批量语义推荐（复用 RAG） | ✅ MVP |
| 坐席辅助面板（转写区 + 推荐卡片） | ✅ MVP |
| 轻量会话沉淀（转写 + 推荐记录） | ✅ MVP |
| 真实 CC 系统对接 | ❌ 后续迭代 |
| 真实 ASR 服务对接 | ❌ 后续迭代 |
| 坐席反馈（采纳/忽略/置顶） | ❌ 后续迭代（消息格式已预留） |
| 脚本分步指引 / 流程图展示 | ❌ 后续迭代 |
| 历史会话列表浏览 | ❌ 后续迭代 |
| 多厂商 CC 适配层 | ❌ 后续迭代 |

---

## 2. 整体架构

```text
┌─────────────┐    转写文本     ┌──────────────────┐   /ai/qa    ┌─────────────┐
│ Mock ASR    │ ──────────────→ │  Java 后端       │ ──────────→ │ Python AI   │
│ 模拟器      │  (REST POST)   │  (WebSocket Hub) │ ←────────── │ 服务        │
└─────────────┘                 │                  │  推荐结果   └─────────────┘
                                │  ┌─ transcript   │
                                │  │  汇总缓冲     │
                                │  │  定时触发     │
                                │  │  会话沉淀     │
                                └──┼───────────────┘
                                   │ WebSocket
                                   ↓
                            ┌──────────────┐
                            │ Qt 坐席面板   │
                            │ (转写区+推荐) │
                            └──────────────┘
```

**核心数据流**：

1. Mock ASR 模拟器定时向 Java `POST /realtime/asr/push` 发送模拟转写文本（含说话人标识 customer/agent）
2. Java 将转写文本追加到当前会话缓冲区（写入 qa_message）
3. 每隔 N 秒，Java 汇总最近的未检索文本，调用 Python `/ai/qa` 做语义检索
4. Java 将推荐结果 + 最新转写文本通过 WebSocket 推送给 Qt 客户端
5. Qt 坐席面板实时更新转写区和推荐卡片
6. 通话结束时，会话标记 ENDED，转写文本完整保留在 qa_message 中

**会话生命周期**：

1. `POST /realtime/session/start` → 创建会话，返回 sessionId
2. ASR 推送 + 推荐循环（WebSocket 保持连接）
3. `POST /realtime/session/end` → 结束会话

---

## 3. 数据模型

### 3.1 复用现有表

| 模块 10 数据 | 复用表 | 映射方式 |
|---|---|---|
| 实时辅助会话 | `qa_session` | `user_id` = 坐席 ID，`title` = "实时辅助-{主叫号码}-{时间}"，`category_id` = NULL |
| ASR 转写片段 | `qa_message` | `session_id` 关联，`role` = `customer` / `agent`，`content` = 转写文本，按 `id` 自增保证顺序 |
| 推荐记录 | `qa_reference` | `message_id` 关联触发消息，`article_id` / `article_title` / `score` 完全匹配 |

**会话类型区分**：`qa_session.title` 前缀 `"实时辅助-"` 标识实时会话，与普通问答会话区分。查询时 `WHERE title LIKE '实时辅助-%'` 过滤。

### 3.2 新增表

**`rt_active_session` — 活跃实时会话**

```sql
CREATE TABLE rt_active_session (
    id              BIGINT AUTO_INCREMENT PRIMARY KEY,
    session_id      BIGINT NOT NULL COMMENT '关联 qa_session.id',
    agent_user_id   BIGINT NOT NULL COMMENT '坐席用户 ID',
    caller_number   VARCHAR(64)  COMMENT '主叫号码',
    cc_call_id      VARCHAR(128) COMMENT 'CC 通话 ID（Mock 阶段可空）',
    status          VARCHAR(20) NOT NULL DEFAULT 'ACTIVE' COMMENT 'ACTIVE / ENDED',
    last_check_time DATETIME COMMENT '上次推荐检查时间',
    create_time     DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_agent (agent_user_id),
    INDEX idx_status (status),
    UNIQUE KEY uk_session (session_id)
) COMMENT '活跃实时辅助会话';
```

**设计要点**：
- 会话结束后记录保留（轻量沉淀），不做软删除
- `last_check_time` 用于推荐循环，标记上次检查位置，避免重复检索
- `session_id` 唯一键确保一个 qa_session 只对应一个活跃会话

### 3.3 Flyway V12

- 建 `rt_active_session` 表
- 授 `realtime:assist` 权限：菜单 600（实时辅助，目录）+ 菜单 601（坐席辅助，route `realtime`）+ 按钮 6011（`realtime:assist`），幂等授予 ADMIN(1) / AGENT(4)

---

## 4. 后端 API

### 4.1 REST 接口

**实时会话管理**（`com.kb.realtime.RtController`，前缀 `/realtime`）：

| 方法 | 路径 | 权限 | 说明 |
|---|---|---|---|
| POST | `/realtime/session/start` | `realtime:assist` | 开始会话，传入 callerNumber，创建 qa_session + rt_active_session，返回 sessionId |
| POST | `/realtime/session/end` | `realtime:assist` | 结束会话，更新 qa_session.status=ENDED，清除 rt_active_session |
| GET | `/realtime/session/active` | `realtime:assist` | 查询当前坐席的活跃会话（断线重连用） |
| POST | `/realtime/asr/push` | 无（内部调用） | 推送转写片段，传入 sessionId + speaker + content，写入 qa_message |
| GET | `/realtime/config` | `realtime:assist` | 获取实时辅助配置 |
| PUT | `/realtime/config` | `ai:config` | 更新配置（管理员权限） |

### 4.2 WebSocket 协议

**端点**：`ws://host/api/ws/realtime/{sessionId}?token={jwt}`

**握手**：通过 URL 参数传 JWT token，`WebSocketConfig` 在握手阶段验证 token 有效性。

**服务端 → 客户端推送消息格式**：

```json
// 转写文本推送
{
  "type": "transcript",
  "data": {
    "speaker": "customer",
    "content": "我想退订上个月的套餐",
    "seqNo": 5
  }
}

// 推荐结果推送
{
  "type": "recommendation",
  "data": {
    "triggerText": "退订 套餐",
    "articles": [
      {"articleId": 4002, "title": "套餐退订流程", "score": 0.85, "rank": 1},
      {"articleId": 4005, "title": "退费话术模板", "score": 0.72, "rank": 2}
    ]
  }
}

// 会话结束通知
{
  "type": "session_end",
  "data": {"sessionId": 123}
}
```

**客户端 → 服务端消息**（预留，MVP 暂不实现）：

```json
// 坐席反馈（后续迭代）
{
  "type": "feedback",
  "data": {"articleId": 4002, "action": "adopt"}
}
```

### 4.3 推荐循环逻辑

```
@Scheduled(fixedDelay = 5000)  // 默认 5 秒，可配置
RtRecommendService.triggerRecommendations():
  1. SELECT * FROM rt_active_session WHERE status = 'ACTIVE'
  2. 对每个活跃会话：
     a. SELECT * FROM qa_message
        WHERE session_id = ? AND role = 'customer' AND create_time > last_check_time
        ORDER BY id ASC
     b. 若有新消息：拼接 content 为 triggerText
     c. 调用 QaService（复用 RAG 检索）获取推荐结果
     d. 将推荐结果写入 qa_reference（message_id = 最后一条消息的 id）
     e. 更新 rt_active_session.last_check_time
     f. 通过 WebSocket 推荐推送给客户端
  3. 若会话已无活跃 WebSocket 连接，跳过推送（不报错）
```

---

## 5. Java 后端包结构

```
com.kb.realtime
├── RtSession.java              // rt_active_session 实体
├── RtSessionMapper.java        // MyBatis-Plus Mapper
├── RtSessionService.java       // 会话管理（start/end/active）
├── RtAsrService.java           // ASR 推送处理（写 qa_message）
├── RtRecommendService.java     // 推荐循环（定时任务 + 调用 QaService）
├── RtWebSocketHandler.java     // WebSocket 消息推送
├── RtWebSocketConfig.java      // WebSocket 端点配置 + JWT 握手验证
├── RtController.java           // REST 接口
└── package-info.java           // 更新：从"骨架占位"改为"已实现"
```

**关键依赖**：
- `QaService`（复用 RAG 推荐能力）
- `qa_session` / `qa_message` / `qa_reference` 的 Mapper（复用现有表）
- `spring-boot-starter-websocket`（新增 Maven 依赖）

---

## 6. Qt 客户端设计

### 6.1 页面结构

在 `MainWindow` 导航树新增菜单项「坐席辅助」（route `realtime`），点击进入 `AgentAssistPage`。

```text
┌─────────────────────────────────────────────────────┐
│  坐席辅助面板                            [开始会话]  │
├───────────────────────────┬─────────────────────────┤
│  📞 通话信息               │  📋 实时转写             │
│  主叫: 138****1234        │  ─────────────────────  │
│  状态: 通话中              │  客户: 我想退订套餐      │
│  时长: 02:35               │  坐席: 好的，请问...     │
│                           │  客户: 上个月办的那个     │
│  ⚙ 推荐设置               │  ─────────────────────  │
│  间隔: 5s  条数: 3         │                         │
├───────────────────────────┼─────────────────────────┤
│  💡 推荐知识                │                         │
│  ┌─────────────────────┐  │                         │
│  │ 1. 套餐退订流程      │  │                         │
│  │    匹配度: 85%       │  │                         │
│  │    [查看] [复制话术]  │  │                         │
│  ├─────────────────────┤  │                         │
│  │ 2. 退费话术模板      │  │                         │
│  │    匹配度: 72%       │  │                         │
│  │    [查看] [复制话术]  │  │                         │
│  └─────────────────────┘  │                         │
└───────────────────────────┴─────────────────────────┘
```

### 6.2 组件拆分

| 组件 | 类 | 职责 |
|---|---|---|
| 坐席辅助页 | `AgentAssistPage` | 主页面，组合以下三个区域，管理会话生命周期 |
| 通话信息区 | 内嵌在 AgentAssistPage | 显示主叫号码、通话状态、时长计时器（QTimer） |
| 实时转写区 | `TranscriptWidget` | 滚动展示客户/坐席对话，自动滚动到底部 |
| 推荐卡片区 | `RecommendationWidget` | 展示推荐知识卡片，支持查看/复制话术 |

### 6.3 核心交互流程

1. **开始会话**：点击「开始会话」→ 弹窗输入主叫号码 → 调用 `POST /realtime/session/start` → 建立 WebSocket 连接 → 按钮变为「结束会话」
2. **接收转写**：WebSocket 收到 `transcript` 消息 → 追加到转写区，自动滚动
3. **接收推荐**：WebSocket 收到 `recommendation` 消息 → 更新推荐卡片列表
4. **查看知识**：点击推荐卡片「查看」→ 打开 `ArticleDetailDialog`（复用现有详情弹窗）
5. **复制话术**：点击「复制话术」→ 将知识正文纯文本复制到剪贴板
6. **结束会话**：点击「结束会话」→ 调用 `POST /realtime/session/end` → 断开 WebSocket → 转写区保留（只读）→ 推荐区清空
7. **断线重连**：WebSocket 断开时自动重连（指数退避，最多 3 次），重连后调用 `GET /realtime/session/active` 恢复状态

### 6.4 权限控制

- 新增权限码 `realtime:assist`，菜单项按此权限显隐
- 坐席角色（AGENT）默认授予
- 管理员默认可见

### 6.5 样式

复用 `app.qss` 既有样式，新增：
- `#TranscriptArea` — 转写区容器（浅色背景、圆角）
- `#CustomerBubble` / `#AgentBubble` — 转写气泡（左对齐/右对齐，与 QA 页面 ChatUserBubble/ChatBotBubble 风格一致）
- `#RecommendCard` — 推荐卡片（与 QA 页面 CitationCard 风格一致）

---

## 7. Mock ASR 模拟器

### 7.1 概述

独立 Python 脚本 `ai-service/scripts/mock_asr.py`，模拟 CC 系统的 ASR 推送行为。

### 7.2 用法

```bash
conda activate kb-ai
python scripts/mock_asr.py --base-url http://localhost:8080/api --interval 3
```

### 7.3 行为

1. 调用 `POST /realtime/session/start` 创建会话（自动登录获取 JWT）
2. 每隔 N 秒从预设话术库中按序取出一段客户/坐席对话
3. 调用 `POST /realtime/asr/push` 推送转写片段
4. 对话推完后调用 `POST /realtime/session/end` 结束会话

### 7.4 预设话术库

`ai-service/scripts/mock_asr_dialogues.json`：包含 3-5 组客服场景对话，覆盖退订、投诉、咨询等场景，每组 6-10 轮对话。

```json
[
  [
    {"speaker": "customer", "content": "你好，我想退订上个月办的套餐"},
    {"speaker": "agent", "content": "好的，请问您要退订哪个套餐呢"},
    {"speaker": "customer", "content": "就是那个99元的流量套餐"},
    {"speaker": "agent", "content": "好的，我帮您查询一下"},
    {"speaker": "customer", "content": "办完之后流量根本不够用，太坑了"}
  ]
]
```

---

## 8. 错误处理

| 场景 | 处理方式 |
|---|---|
| WebSocket 连接断开 | Qt 客户端自动重连（指数退避，最多 3 次），重连后调用 `GET /realtime/session/active` 恢复状态 |
| Python AI 服务不可用 | 推荐循环跳过本次，WebSocket 推送空推荐 + 提示"知识服务暂不可用"，不阻断转写展示 |
| 推荐无命中 | 推送空 articles 列表，Qt 面板显示"暂无匹配知识" |
| ASR 推送到达但无活跃会话 | 返回 `code=1001 无活跃会话`，Mock 模拟器收到错误后停止推送 |
| 重复开始会话 | 同一坐席已有活跃会话时返回已有 sessionId，不重复创建 |

---

## 9. 测试策略

### 9.1 后端验证

1. `mvn compile` 编译通过
2. 接口冒烟：session start → asr push → 推荐触发 → session end 全流程
3. WebSocket 推送验证：用 wscat 或 Qt 客户端连接，确认 transcript/recommendation 消息到达

### 9.2 Qt 客户端验证

1. CMake/MinGW 构建通过
2. 坐席面板点测：开始会话 → 转写区实时更新 → 推荐卡片更新 → 结束会话

### 9.3 端到端验证

1. 启动 Java + Python + Mock ASR
2. Qt 客户端进入坐席辅助页，点击开始会话
3. Mock ASR 自动推送对话 → 面板实时显示转写 + 推荐知识
4. 点击推荐卡片「查看」→ 打开知识详情
5. 结束会话 → 转写保留、推荐清空

---

## 10. 新增依赖

### Java 后端

```xml
<!-- pom.xml 新增 -->
<dependency>
    <groupId>org.springframework.boot</groupId>
    <artifactId>spring-boot-starter-websocket</artifactId>
</dependency>
```

### Qt 客户端

```cmake
# CMakeLists.txt 新增
find_package(Qt6 REQUIRED COMPONENTS WebSockets)
target_link_libraries(knowledgeAnswer PRIVATE Qt6::WebSockets)
```

---

## 11. Flyway V12 迁移内容

```sql
-- 建表
CREATE TABLE rt_active_session (...);

-- 菜单与权限（幂等）
-- 目录 600：实时辅助
-- 菜单 601：坐席辅助（route realtime）
-- 按钮 6011：realtime:assist
-- 授予 ADMIN(1) / AGENT(4)
```
