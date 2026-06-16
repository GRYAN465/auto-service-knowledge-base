# 智能客服知识库系统 — API 契约（草案 v0.1）

> 配套《总体开发计划.md》§5、《数据库表清单.md》。本文件定通用约定 + 一期接口草案，作为前后端联调依据。逐模块开发时在此细化字段。
> 二期接口（问答 / 实时辅助）只列规划，标 TODO。
>
> 最近更新：2026-06-16

---

## 1. 通用约定

- **Base URL**：`http://<host>:8080/api`（前缀 `/api`，可配）。
- **协议**：HTTP + JSON，`Content-Type: application/json`；文件上传 `multipart/form-data`。
- **统一响应体**：

  ```json
  { "code": 0, "message": "ok", "data": {} }
  ```

  `code = 0` 成功；非 0 为业务错误。当前业务异常/参数校验由统一响应体承载（通常 HTTP 200 + `code != 0`）；未认证由安全链返回 HTTP 401；无权限返回 `code=2003`（安全链拦截场景为 HTTP 403）。客户端以 `code` 判断业务成功与否。

- **错误码段**（约定，可扩展）：

  | 段 | 含义 |
  |---|---|
  | 0 | 成功 |
  | 1xxx | 通用/参数/校验 |
  | 2xxx | 鉴权与权限（2001 未登录、2002 token 失效、2003 无权限） |
  | 3xxx | 系统管理 |
  | 4xxx | 知识管理 |
  | 5xxx | 检索/应用/互动 |
  | 6xxx | 开放 API |

- **鉴权**：除 `/auth/login` 外，请求头带 `Authorization: Bearer <jwt>`。
- **分页**：请求 `?page=1&pageSize=20`（或 body）；响应 `data`：

  ```json
  { "total": 100, "page": 1, "pageSize": 20, "list": [] }
  ```

- **时间**：字符串 `yyyy-MM-dd HH:mm:ss`。
- **ID**：字符串或数字均按 BIGINT 处理。

---

## 2. 鉴权（模块 1）

| 方法 | 路径 | 说明 |
|---|---|---|
| POST | `/auth/login` | 入参 `{username,password}`；从 `sys_user` 查账号并用 BCrypt 校验密码；校验账号启用/有效期；出参 `{token, expiresIn}` |
| POST | `/auth/logout` | 无状态退出；客户端丢弃当前 token，服务端不维护 token 黑名单 |
| GET | `/auth/me` | 从 DB 装配当前用户 + 角色 + **菜单树 + 权限码列表**（驱动客户端导航与按钮） |

`/auth/me` 响应 `data` 形态：

```json
{
  "user": { "id": 1, "username": "admin", "realName": "管理员", "avatar": "", "orgId": 1 },
  "roles": ["ADMIN"],
  "permissions": ["system:user:list", "knowledge:article:create", "..."],
  "menus": [
    { "name": "dashboard", "title": "首页仪表盘", "icon": "home", "children": [] },
    { "name": "knowledge.search", "title": "知识检索", "icon": "search", "children": [] }
  ]
}
```

> `menus[].name` 对应客户端 `PageRouter` 注册名（见 §14.1 页面树）；`permissions` 控制按钮可见/可用。

---

## 3. 系统管理（模块 2）

> 统一 CRUD 套路：`GET /xxx`（分页查询）、`GET /xxx/{id}`、`POST /xxx`、`PUT /xxx/{id}`、`DELETE /xxx/{id}`。下表只列资源与差异点。

| 资源 | 路径前缀 | 权限码前缀 | 备注 |
|---|---|---|---|
| 机构/部门 | `/system/org` | `system:org:*` | 树查询 `GET /system/org/tree` |
| 人员 | `/system/user` | `system:user:*` | 改密 `PUT /system/user/{id}/password`；启停 `PUT /system/user/{id}/status`；导入 `POST /system/user/import` |
| 角色 | `/system/role` | `system:role:*` | 分配权限 `PUT /system/role/{id}/permissions` |
| 菜单/权限 | `/system/permission` | `system:permission:*` | 树查询 `GET /system/permission/tree` |
| 登录日志 | `/system/log/login` | `system:log:login` | 只读分页 |
| 操作日志 | `/system/log/operation` | `system:log:operation` | 只读分页 |

---

## 4. 知识基础 + 管理（模块 3、4）

| 资源 | 路径前缀 | 权限码前缀 | 备注 |
|---|---|---|---|
| 分类 | `/knowledge/category` | `knowledge:category:*` | 树 `GET .../tree` |
| 标签 | `/knowledge/tag` | `knowledge:tag:*` | |
| 知识 | `/knowledge/article` | `knowledge:article:*` | 见下方明细 |
| 附件 | `/knowledge/attachment` | `knowledge:attachment:*` | 上传 `POST .../upload`；下载 `GET .../{id}/download` |

知识主资源关键接口：

| 方法 | 路径 | 说明 |
|---|---|---|
| GET | `/knowledge/article` | 分页（条件：标题/分类/标签/状态/类型） |
| GET | `/knowledge/article/{id}` | 详情（富文本 + 附件 + 标签） |
| POST | `/knowledge/article` | 新建（草稿） |
| PUT | `/knowledge/article/{id}` | 编辑（生成新版本） |
| DELETE | `/knowledge/article/{id}` | 逻辑删除 |
| POST | `/knowledge/article/{id}/submit` | 提交审核 |
| POST | `/knowledge/article/{id}/audit` | 审核 `{auditStatus, opinion}` |
| POST | `/knowledge/article/{id}/online` | 上线 |
| POST | `/knowledge/article/{id}/offline` | 下线 `{reason}` |
| GET | `/knowledge/article/{id}/versions` | 版本列表 |
| POST | `/knowledge/article/import` | 模板导入 |
| POST | `/knowledge/article/download` | 批量下载 `{ids}` → 压缩包 |

---

## 5. 检索 + 详情 + 互动（模块 5、6）

| 方法 | 路径 | 权限码 | 说明 |
|---|---|---|---|
| GET | `/search/article` | `knowledge:search` | 关键词（标题/正文）+ 分类/标签筛选，分页 |
| GET | `/knowledge/article/{id}/view` | — | 查看详情并埋点浏览 |
| POST | `/interaction/favorite` / DELETE `/interaction/favorite/{articleId}` | `interaction:favorite` | 收藏/取消 |
| GET | `/interaction/favorite` | | 我的收藏 |
| POST | `/interaction/like` | `interaction:like` | `{targetType,targetId,type}` 赞/踩 |
| POST | `/interaction/comment` / GET `/interaction/comment?articleId=` | `interaction:comment` | 评论/列表 |
| POST | `/interaction/share` | `interaction:share` | `{articleId,toUserId,message}` + 站内通知 |
| GET | `/interaction/share/inbox` | | 收到的分享 |

> 朗读（TTS）在客户端本地用 Qt TextToSpeech，无需后端接口。

---

## 6. 数据统计 + 开放 API（模块 7、8）

| 方法 | 路径 | 权限码 | 说明 |
|---|---|---|---|
| GET | `/statistics/overview` | `statistics:view` | 仪表盘汇总 |
| GET | `/statistics/hot-article` | `statistics:view` | 热门知识 |
| GET | `/statistics/hot-keyword` | `statistics:view` | 热门搜索词 |

开放 API（对外，AppKey 鉴权，前缀 `/open`，独立于 `/api`）：

| 方法 | 路径 | 说明 |
|---|---|---|
| 管理 | `/api/openapi/app` (CRUD) | AppKey/限流配置，权限码 `openapi:app:*` |
| GET | `/open/v1/search` | 对外检索（AppKey + 签名 + 限流 + 调用日志） |
| GET | `/open/v1/article/{id}` | 对外详情 |
| POST | `/open/v1/qa` | 对外问答（转发二期 AI，先占位 501） |

---

## 7. 健康检查 / 文档

- `GET /actuator/health` → 健康状态。
- Swagger UI：`/swagger-ui.html`（springdoc-openapi）。

---

## 8. 二期接口（TODO）

- **问答（模块 9）**：`POST /ai/qa`（Java 编排，内部调 FastAPI）；FastAPI 侧 `POST /ai/parse`、`/ai/embed`、`/ai/qa`。
- **实时辅助（模块 10）**：WebSocket `/ws/agent`（实时转写推送 + 推荐卡片）；CC/ASR 配置 `/realtime/config`。

> 二期接口待模块 9、10 启动时细化。FastAPI 契约见《总体开发计划.md》§4。
