# 模块 10 客服实时辅助 MVP — 实现计划

> 基于设计文档 `2026-06-24-module10-realtime-agent-assist-design.md`，按依赖顺序排列。
> 每步标注涉及文件和验证方式。

---

## Phase 1：基础设施（Java 后端 + Flyway）

### Step 1.1：添加 Maven 依赖

**文件**：`server/pom.xml`

在 `<dependencies>` 中新增：
```xml
<dependency>
    <groupId>org.springframework.boot</groupId>
    <artifactId>spring-boot-starter-websocket</artifactId>
</dependency>
```

**验证**：`mvn compile` 通过。

---

### Step 1.2：Flyway V12 迁移脚本

**文件**：`server/src/main/resources/db/migration/V12__module10_realtime.sql`

内容：
1. 建 `rt_active_session` 表（完整 DDL，见设计文档 §3.2）
2. 幂等插入菜单目录 600「实时辅助」
3. 幂等插入菜单 601「坐席辅助」（route `realtime`，父目录 600）
4. 幂等插入按钮权限 6011（`realtime:assist`，父菜单 601）
5. 幂等授予 ADMIN(1) 和 AGENT(4) 对应权限

**参考**：复用 `V10__module9_qa.sql` 和 `V11__module9_ai_config.sql` 的菜单/权限种子写法（`INSERT ... SELECT ... WHERE NOT EXISTS`）。

**验证**：后端启动后 Flyway 迁至 v12，`SHOW TABLES LIKE 'rt_active_session'` 返回 1 行。

---

### Step 1.3：RtSession 实体

**文件**：`server/src/main/java/com/kb/realtime/entity/RtSession.java`

- `@Data` + `@TableName("rt_active_session")`
- Style B（不继承 BaseEntity，自建 `@TableId(type = IdType.AUTO) Long id`）
- 字段：`sessionId`, `agentUserId`, `callerNumber`, `ccCallId`, `status`, `lastCheckTime`, `createTime`

---

### Step 1.4：RtSessionMapper

**文件**：`server/src/main/java/com/kb/realtime/mapper/RtSessionMapper.java`

- `extends BaseMapper<RtSession>`，空接口，与项目其他 Mapper 一致

---

### Step 1.5：SecurityConfig 放行 WebSocket 路径

**文件**：`server/src/main/java/com/kb/security/SecurityConfig.java`

在 `securityFilterChain` 的 `requestMatchers(HttpMethod.GET, ...)` 列表中，或 `.permitAll()` 列表中新增：
```
"/ws/realtime/**"
```

WebSocket 握手是 HTTP GET 请求，需要放行才能建立连接。JWT 验证在 `RtWebSocketConfig` 的握手拦截器中单独处理。

---

## Phase 2：Java 后端核心服务

### Step 2.1：RtSessionService — 会话管理

**文件**：`server/src/main/java/com/kb/realtime/RtSessionService.java`

- `@Slf4j` + `@Service`，构造注入 `RtSessionMapper`、`QaSessionMapper`、`QaMessageMapper`
- 方法：
  - `startSession(Long agentUserId, String callerNumber)` → 创建 `QaSession`（title="实时辅助-{callerNumber}-{时间}"，userId=agentUserId，categoryId=null，status=ACTIVE）+ 创建 `RtSession`，返回 sessionId
  - `endSession(Long sessionId)` → 更新 QaSession.status=ENDED，更新 RtSession.status=ENDED
  - `getActiveSession(Long agentUserId)` → 查 RtSession WHERE agentUserId AND status=ACTIVE
  - `getAllActiveSessions()` → 查 RtSession WHERE status=ACTIVE（供推荐循环使用）

---

### Step 2.2：RtAsrService — ASR 推送处理

**文件**：`server/src/main/java/com/kb/realtime/RtAsrService.java`

- `@Slf4j` + `@Service`，构造注入 `RtSessionMapper`、`QaMessageMapper`
- 方法：
  - `pushSegment(Long sessionId, String speaker, String content)` → 校验 session 存在且 ACTIVE → 写入 QaMessage（sessionId, role=speaker, content）→ 返回写入的 message id
- 异常：session 不存在或非 ACTIVE → 抛 `BusinessException(1001, "无活跃会话")`

---

### Step 2.3：RtWebSocketHandler — WebSocket 消息推送

**文件**：`server/src/main/java/com/kb/realtime/RtWebSocketHandler.java`

- 继承 `TextWebSocketHandler`
- 维护 `ConcurrentHashMap<Long, WebSocketSession>` 映射 sessionId → WebSocket 连接
- `afterConnectionEstablished()` → 从 URL 路径提取 sessionId，注册到连接池
- `afterConnectionClosed()` → 从连接池移除
- `sendToSession(Long sessionId, String jsonMessage)` → 查连接池，存在则 `sendMessage()`
- `sendTranscript(Long sessionId, String speaker, String content, int seqNo)` → 构造 transcript JSON → sendToSession
- `sendRecommendation(Long sessionId, String triggerText, List<ArticleRecommendation> articles)` → 构造 recommendation JSON → sendToSession
- `sendSessionEnd(Long sessionId)` → 构造 session_end JSON → sendToSession
- `isConnected(Long sessionId)` → 检查连接池中是否存在且 isOpen

---

### Step 2.4：RtWebSocketConfig — WebSocket 端点配置

**文件**：`server/src/main/java/com/kb/realtime/RtWebSocketConfig.java`

- 实现 `WebSocketConfigurer`
- `registerWebSocketHandlers()` → 注册 `RtWebSocketHandler` 到路径 `/ws/realtime/{sessionId}`
- 添加 `HandshakeInterceptor`：
  - `beforeHandshake()` → 从 URL 参数 `?token=xxx` 提取 JWT → 用 `JwtTokenProvider` 验证 → 有效则放行
  - `afterHandshake()` → 空实现

---

### Step 2.5：RtRecommendService — 推荐循环

**文件**：`server/src/main/java/com/kb/realtime/RtRecommendService.java`

- `@Slf4j` + `@Service` + `@EnableScheduling`
- 构造注入 `RtSessionService`、`QaMessageMapper`、`QaService`、`RtWebSocketHandler`
- 常量：`TRIGGER_INTERVAL = 5000`（5秒），`TOP_N = 3`
- `@Scheduled(fixedDelay = 5000)` 方法 `triggerRecommendations()`：
  1. 获取所有活跃会话 `rtSessionService.getAllActiveSessions()`
  2. 对每个会话：
     a. 查 `QaMessage WHERE sessionId AND role='customer' AND createTime > lastCheckTime ORDER BY id ASC`
     b. 若无新消息，跳过
     c. 拼接 content 为 triggerText
     d. 调用 `QaService.ask(triggerText, null, TOP_N, null)` 获取推荐
     e. 若 WebSocket 连接存在，推送 recommendation 消息
     f. 更新 `rtSession.lastCheckTime = now()`
  3. 异常处理：单个会话失败不影响其他会话（try-catch 包裹循环体）

---

### Step 2.6：RtController — REST 接口

**文件**：`server/src/main/java/com/kb/realtime/RtController.java`

- `@Tag(name = "实时辅助")` + `@RestController` + `@RequestMapping("/realtime")`
- 构造注入 `RtSessionService`、`RtAsrService`
- 接口：
  - `POST /session/start` → `@PreAuthorize("hasAuthority('realtime:assist')")` → `RtSessionService.startSession(userId, callerNumber)` → 返回 sessionId
  - `POST /session/end` → `@PreAuthorize("hasAuthority('realtime:assist')")` → `RtSessionService.endSession(sessionId)`
  - `GET /session/active` → `@PreAuthorize("hasAuthority('realtime:assist')")` → `RtSessionService.getActiveSession(userId)`
  - `POST /asr/push` → `@PreAuthorize("hasAuthority('realtime:assist')")` → `RtAsrService.pushSegment(sessionId, speaker, content)`

**请求 DTO**：
- `RtStartRequest`：`@NotBlank String callerNumber`
- `RtEndRequest`：`@NotNull Long sessionId`
- `RtAsrPushRequest`：`@NotNull Long sessionId`, `@NotBlank String speaker`, `@NotBlank String content`

**响应 VO**：
- `RtSessionVO`：`sessionId`, `callerNumber`, `status`, `startTime`

---

### Step 2.7：更新 package-info.java

**文件**：`server/src/main/java/com/kb/realtime/package-info.java`

将「骨架占位，见…」改为「已实现，见…」。

---

## Phase 3：Qt 客户端

### Step 3.1：ApiClient 扩展 WebSocket 支持

**文件**：`knowledgeAnswer/core/network/ApiClient.h` / `.cpp`

ApiClient 当前只有 REST 方法。WebSocket 连接不走 ApiClient，直接在 AgentAssistPage 中使用 `QWebSocket`。

无需修改 ApiClient，但需在 CMakeLists.txt 添加 `WebSockets` 模块。

---

### Step 3.2：CMakeLists.txt 添加依赖和源文件

**文件**：`knowledgeAnswer/CMakeLists.txt`

1. `find_package(Qt6 REQUIRED COMPONENTS WebSockets)`
2. `target_link_libraries` 添加 `Qt6::WebSockets`
3. `PROJECT_SOURCES` 添加：
   - `app/AgentAssistPage.h  app/AgentAssistPage.cpp`
   - `app/TranscriptWidget.h  app/TranscriptWidget.cpp`
   - `app/RecommendationWidget.h  app/RecommendationWidget.cpp`

---

### Step 3.3：TranscriptWidget — 实时转写区

**文件**：`knowledgeAnswer/app/TranscriptWidget.h` / `.cpp`

- `namespace kb`，继承 `QWidget`，`Q_OBJECT`
- 内部使用 `QScrollArea` + `QVBoxLayout`（与 QaPage 消息区同模式）
- 公有方法：
  - `appendSegment(const QString &speaker, const QString &content)` → 创建气泡 QLabel（customer 左对齐 objectName="CustomerBubble"，agent 右对齐 objectName="AgentBubble"）→ 插入 layout → 滚动到底部
  - `clear()` → 清空所有气泡
  - `setReadOnly(bool)` → 只读模式（会话结束后）
- 自动滚动：`QTimer::singleShot(0, scrollArea, &QAbstractScrollArea::ensureVisible)`

---

### Step 3.4：RecommendationWidget — 推荐卡片区

**文件**：`knowledgeAnswer/app/RecommendationWidget.h` / `.cpp`

- `namespace kb`，继承 `QWidget`，`Q_OBJECT`
- 内部使用 `QVBoxLayout` 动态添加卡片
- 信号：
  - `viewRequested(qint64 articleId)` — 点击「查看」时发射
- 公有方法：
  - `updateRecommendations(const QJsonArray &articles)` → 清空旧卡片 → 逐个创建推荐卡片（objectName="RecommendationCard"）：序号 + 标题 + 匹配度百分比 + [查看] [复制话术] 按钮
  - `clear()` → 清空所有卡片
- 「查看」按钮 → emit `viewRequested(articleId)`
- 「复制话术」按钮 → 调用 `ApiClient::instance().get("/knowledge/article/" + id, ...)` 获取正文 → 纯文本写入 `QApplication::clipboard()`

---

### Step 3.5：AgentAssistPage — 坐席辅助主页面

**文件**：`knowledgeAnswer/app/AgentAssistPage.h` / `.cpp`

- `namespace kb`，继承 `QWidget`，`Q_OBJECT`
- 构造：`explicit AgentAssistPage(const QString &title, QWidget *parent = nullptr)`
- UI 布局（`buildUi()`）：
  - 顶部工具栏：标题 + 「开始会话」/「结束会话」按钮（objectName="PrimaryButton"）
  - 左侧面板（QGroupBox）：
    - 通话信息区（主叫号码 QLabel、状态 QLabel、时长 QLabel + QTimer 每秒更新）
    - 建议设置显示（硬编码 "间隔: 5s  条数: 3"，只读）
  - 右侧面板：
    - 上方：`TranscriptWidget`（占大部分空间）
    - 下方：`RecommendationWidget`
  - 使用 `QSplitter(Qt::Vertical)` 分割转写区和推荐区
  - 使用 `QSplitter(Qt::Horizontal)` 分割左侧面板和右侧主区域

- 成员变量：
  - `QPushButton *m_toggleBtn`
  - `QLabel *m_callerLabel, *m_statusLabel, *m_durationLabel`
  - `TranscriptWidget *m_transcript`
  - `RecommendationWidget *m_recommendations`
  - `QWebSocket *m_ws`（WebSocket 连接）
  - `qint64 m_sessionId`（当前会话 ID）
  - `QTimer *m_durationTimer`（通话时长计时器）
  - `QDateTime m_sessionStartTime`
  - `int m_reconnectAttempts`（重连计数）

- 核心方法：
  - `startSession()` → 弹 `QInputDialog` 获取主叫号码 → `POST /realtime/session/start` → 成功后建立 WebSocket → 按钮变「结束会话」→ 启动时长计时器
  - `endSession()` → `POST /realtime/session/end` → 关闭 WebSocket → 转写区只读 → 推荐区清空 → 按钮变「开始会话」
  - `connectWebSocket(sessionId)` → 创建 `QWebSocket` → 连接 `wss://host/api/ws/realtime/{sessionId}?token={jwt}` → 连接成功后 `m_reconnectAttempts = 0`
  - `onWebSocketMessage(const QString &msg)` → 解析 JSON → 按 type 分发：
    - `transcript` → `m_transcript->appendSegment(speaker, content)`
    - `recommendation` → `m_recommendations->updateRecommendations(articles)`
    - `session_end` → `endSession()`
  - `onWebSocketDisconnected()` → 自动重连（指数退避 1s/2s/4s，最多 3 次）
  - `onViewArticle(qint64 articleId)` → `ArticleDetailDialog(articleId, this).exec()`

- 异步回调模式：全部使用 `QPointer<AgentAssistPage> self(this)` + `if (!self) return;` 守卫

---

### Step 3.6：MainWindow 注册路由

**文件**：`knowledgeAnswer/app/MainWindow.cpp`

在 `registerPages()` 的 if-else 链中新增：
```cpp
if (name == QStringLiteral("realtime")) {
    return new AgentAssistPage(title);
}
```

在文件顶部添加 `#include "app/AgentAssistPage.h"`。

---

### Step 3.7：QSS 样式补充

**文件**：knowledgeAnswer/resources/styles/app.qss`

新增：
```css
/* 坐席辅助面板 — 转写气泡 */
#CustomerBubble {
    background: #E8F5E9;
    border-radius: 8px;
    padding: 8px 12px;
    margin: 2px 40px 2px 4px;
}
#AgentBubble {
    background: #E3F2FD;
    border-radius: 8px;
    padding: 8px 12px;
    margin: 2px 4px 2px 40px;
}
/* 坐席辅助面板 — 推荐卡片 */
#RecommendationCard {
    background: #FAFAFA;
    border: 1px solid #E0E0E0;
    border-radius: 6px;
    padding: 8px;
    margin: 2px 0;
}
```

---

## Phase 4：Mock ASR 模拟器

### Step 4.1：预设话术库

**文件**：`ai-service/scripts/mock_asr_dialogues.json`

3 组客服场景对话（退订、投诉、咨询），每组 6-10 轮。JSON 数组格式。

---

### Step 4.2：Mock ASR 脚本

**文件**：`ai-service/scripts/mock_asr.py`

- 使用 `requests` 库
- 命令行参数：`--base-url`（默认 `http://localhost:8080/api`）、`--username`（默认 `agent1`）、`--password`（默认 `123456`）、`--interval`（默认 3 秒）
- 流程：
  1. `POST /auth/login` 获取 JWT
  2. `POST /realtime/session/start` 创建会话（callerNumber 随机生成）
  3. 循环：从话术库按序取出每轮对话 → `POST /realtime/asr/push` 推送 → `time.sleep(interval)`
  4. `POST /realtime/session/end` 结束会话
- 错误处理：HTTP 错误打印响应体并退出

---

## Phase 5：验证

### Step 5.1：后端编译验证

```bash
cd server && mvn compile
```

### Step 5.2：Qt 构建验证

Qt Creator 重新 CMake Configure + Build，确认无编译错误。

### Step 5.3：接口冒烟测试

1. 后端启动（Flyway 迁至 v12）
2. admin 登录获取 JWT
3. `POST /realtime/session/start` → 返回 sessionId
4. `POST /realtime/asr/push` → 写入 qa_message
5. 等待 5 秒 → 推荐循环触发
6. `POST /realtime/session/end` → 会话结束

### Step 5.4：端到端验证

1. 启动 Java 后端 + Python AI 服务
2. Qt 客户端登录 → 进入「坐席辅助」页面
3. 运行 `python scripts/mock_asr.py`
4. Qt 面板实时显示转写 + 推荐卡片
5. 点击推荐「查看」→ 打开知识详情
6. 结束会话 → 转写保留、推荐清空

---

## 文件清单汇总

### 新增文件（Java 后端，7 个）
| 文件 | 说明 |
|---|---|
| `server/src/main/resources/db/migration/V12__module10_realtime.sql` | Flyway 迁移 |
| `server/src/main/java/com/kb/realtime/entity/RtSession.java` | 实体 |
| `server/src/main/java/com/kb/realtime/mapper/RtSessionMapper.java` | Mapper |
| `server/src/main/java/com/kb/realtime/RtSessionService.java` | 会话管理 |
| `server/src/main/java/com/kb/realtime/RtAsrService.java` | ASR 推送处理 |
| `server/src/main/java/com/kb/realtime/RtRecommendService.java` | 推荐循环 |
| `server/src/main/java/com/kb/realtime/RtWebSocketHandler.java` | WebSocket 推送 |
| `server/src/main/java/com/kb/realtime/RtWebSocketConfig.java` | WebSocket 配置 |
| `server/src/main/java/com/kb/realtime/RtController.java` | REST 接口 |
| `server/src/main/java/com/kb/realtime/dto/RtStartRequest.java` | 请求 DTO |
| `server/src/main/java/com/kb/realtime/dto/RtEndRequest.java` | 请求 DTO |
| `server/src/main/java/com/kb/realtime/dto/RtAsrPushRequest.java` | 请求 DTO |
| `server/src/main/java/com/kb/realtime/dto/RtSessionVO.java` | 响应 VO |

### 修改文件（Java 后端，3 个）
| 文件 | 改动 |
|---|---|
| `server/pom.xml` | 新增 websocket 依赖 |
| `server/.../security/SecurityConfig.java` | 放行 `/ws/realtime/**` |
| `server/.../realtime/package-info.java` | 更新注释 |

### 新增文件（Qt 客户端，6 个）
| 文件 | 说明 |
|---|---|
| `knowledgeAnswer/app/AgentAssistPage.h` | 坐席辅助页头文件 |
| `knowledgeAnswer/app/AgentAssistPage.cpp` | 坐席辅助页实现 |
| `knowledgeAnswer/app/TranscriptWidget.h` | 转写区头文件 |
| `knowledgeAnswer/app/TranscriptWidget.cpp` | 转写区实现 |
| `knowledgeAnswer/app/RecommendationWidget.h` | 推荐卡片头文件 |
| `knowledgeAnswer/app/RecommendationWidget.cpp` | 推荐卡片实现 |

### 修改文件（Qt 客户端，3 个）
| 文件 | 改动 |
|---|---|
| `knowledgeAnswer/CMakeLists.txt` | 添加 WebSockets 模块 + 源文件 |
| `knowledgeAnswer/app/MainWindow.cpp` | 注册 `realtime` 路由 |
| `knowledgeAnswer/resources/styles/app.qss` | 新增气泡/卡片样式 |

### 新增文件（Python，2 个）
| 文件 | 说明 |
|---|---|
| `ai-service/scripts/mock_asr.py` | Mock ASR 模拟器 |
| `ai-service/scripts/mock_asr_dialogues.json` | 预设话术库 |
