# 智能客服知识库系统 — API 契约（草案 v0.1）

> 配套《总体开发计划.md》§5、《数据库表清单.md》。本文件定通用约定 + 一期接口草案，作为前后端联调依据。逐模块开发时在此细化字段。
> 二期接口（问答 / 实时辅助）只列规划，标 TODO。
>
> 最近更新：2026-06-18

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

- **鉴权**：除 `/auth/login`、开放 API `/open/v1/**`、健康检查/Swagger 外，业务请求头带 `Authorization: Bearer <jwt>`；开放 API 使用 AppKey + HMAC 签名，见 §6.2。
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
| 角色 | `/system/role` | `system:role:*` | 启用角色选项 `GET /system/role/options`；分配权限 `PUT /system/role/{id}/permissions` |
| 菜单/权限 | `/system/permission` | `system:permission:*` | 树查询 `GET /system/permission/tree` |
| 登录日志 | `/system/log/login` | `system:log:login` | 只读分页 |
| 操作日志 | `/system/log/operation` | `system:log:operation` | 只读分页 |

---

## 4. 知识基础 + 管理（模块 3、4）

| 资源 | 路径前缀 | 权限码前缀 | 备注 |
|---|---|---|---|
| 分类 | `/knowledge/category` | `knowledge:category:*` | 树 `GET .../tree`；删除**级联**（连同全部子孙分类）✅ 已实现 |
| 标签 | `/knowledge/tag` | `knowledge:tag:*` | 分页 + 唯一名（`uk_tag_name`）✅ 已实现 |
| 知识 | `/knowledge/article` | `knowledge:article:*` | CRUD + 版本/审核/上下线状态机（模块 4）✅ 已实现，见 §4.3 |
| 附件 | `/knowledge/attachment` | `knowledge:attachment:*` | 上传 `POST .../upload`；下载 `GET .../{id}/download`（模块 4）✅ 已实现，见 §4.4 |

### 4.1 分类（模块 3，已实现）

| 方法 | 路径 | 权限码 | 说明 |
|---|---|---|---|
| GET | `/knowledge/category` 或 `/knowledge/category/tree` | `knowledge:category:list` | 全量分类树（`List<CategoryTreeVO>`：`id/parentId/name/code/sort/status/children`） |
| GET | `/knowledge/category/{id}` | `knowledge:category:list` | 分类详情（`KbCategory`） |
| POST | `/knowledge/category` | `knowledge:category:create` | 新建，入参 `{parentId(=0 为根), name(必填), code, sort, status}` |
| PUT | `/knowledge/category/{id}` | `knowledge:category:update` | 编辑（校验 `parentId != id`） |
| DELETE | `/knowledge/category/{id}` | `knowledge:category:delete` | **级联逻辑删除**：删除该节点及其全部子孙分类（客户端对含下级的分类要求输入「确定删除」二次确认）。文章占用校验留待模块 4 |

### 4.2 标签（模块 3，已实现）

| 方法 | 路径 | 权限码 | 说明 |
|---|---|---|---|
| GET | `/knowledge/tag?page=1&pageSize=20&keyword=` | `knowledge:tag:list` | 分页（`PageResult<KbTag>`：`total/page/pageSize/list`），`keyword` 按名称模糊 |
| GET | `/knowledge/tag/{id}` | `knowledge:tag:list` | 标签详情（`KbTag`） |
| POST | `/knowledge/tag` | `knowledge:tag:create` | 新建，入参 `{name(必填、唯一), color(#RRGGBB), sort}` |
| PUT | `/knowledge/tag/{id}` | `knowledge:tag:update` | 编辑 |
| DELETE | `/knowledge/tag/{id}` | `knowledge:tag:delete` | 逻辑删除 |

> **唯一名错误约定**：活跃重名由服务层 `ensureNameUnique` 拦截，返回 `code=1001 标签名称已存在`；若与**已软删**的同名行冲突（DB `uk_tag_name` 不含 `deleted`），由全局 `DataIntegrityViolationException` 兜底返回 `code=1001 数据已存在或违反唯一约束`（不再冒泡 5000）。此兜底对 org/user/role/tag 等普通唯一索引同样生效。

### 4.3 知识主资源（模块 4，已实现）

知识主资源关键接口（前缀 `/knowledge/article`，权限码前缀 `knowledge:article:`）：

| 方法 | 路径 | 权限码 | 说明 |
|---|---|---|---|
| GET | `/knowledge/article?page=&pageSize=&keyword=&categoryId=&tagId=&status=&knowledgeType=` | `:list` | 分页（`PageResult<ArticleListItemVO>`：含 categoryName/authorName/tagNames），按标题模糊 + 分类/标签/状态/类型筛选（**`categoryId` 含子分类**：展开为「自身 + 全部子孙分类」后 `IN` 过滤，与门户检索一致） |
| GET | `/knowledge/article/{id}` | `:list` | 详情（`ArticleDetailVO`：富文本正文 + 标签[{id,name,color}] + 附件列表） |
| GET | `/knowledge/article/{id}/versions` | `:list` | 版本列表（按 versionNo 倒序） |
| POST | `/knowledge/article` | `:create` | 新建（状态 DRAFT、source MANUAL、author 取当前用户、currentVersion=1，写 v1 版本快照） |
| PUT | `/knowledge/article/{id}` | `:update` | 编辑（**仅 DRAFT/REJECTED 可改**，currentVersion++ 并写新版本快照） |
| DELETE | `/knowledge/article/{id}` | `:delete` | 逻辑删除（同时清理 `kb_article_tag`） |
| POST | `/knowledge/article/{id}/submit` | `:submit` | 提交审核（DRAFT/REJECTED → PENDING_AUDIT，写提交留痕） |
| POST | `/knowledge/article/{id}/audit` | `:audit` | 审核 `{auditStatus(PASS/REJECT), opinion}`（PASS → ONLINE 写 online_time；REJECT → REJECTED；均写 `kb_audit_record`） |
| POST | `/knowledge/article/{id}/online` | `:online` | 重新上线（OFFLINE → ONLINE） |
| POST | `/knowledge/article/{id}/offline` | `:offline` | 下线 `{reason}`（ONLINE → OFFLINE，写 offline_time/reason） |

> 状态机：`status ∈ DRAFT/PENDING_AUDIT/ONLINE/OFFLINE/REJECTED`；非法流转返回 `code=1001`（如「仅草稿或已驳回的知识可编辑」「仅待审核的知识可审核」）。导入 `POST /knowledge/article/import` 与批量 zip 下载 `POST /knowledge/article/download` 本轮延后，留待后续。

### 4.4 附件（模块 4，已实现）

附件接口（前缀 `/knowledge/attachment`），文件经 `FileStorage` 抽象落本地 `kb.storage.local-dir`（`./uploads`，按 `yyyy/MM/uuid.ext`）：

| 方法 | 路径 | 权限码 | 说明 |
|---|---|---|---|
| POST | `/knowledge/attachment/upload?articleId=` | `knowledge:attachment:upload` | `multipart/form-data`（字段 `file`）上传，存盘 + insert `kb_attachment`，自动识别 fileType |
| GET | `/knowledge/attachment?articleId=` | `knowledge:article:list` | 某知识的附件列表 |
| GET | `/knowledge/attachment/{id}/download` | `knowledge:article:list` | 流式下载（`Content-Disposition`，UTF-8 文件名），`download_count++` |
| DELETE | `/knowledge/attachment/{id}` | `knowledge:attachment:delete` | 逻辑删除 + 删盘文件 |

> 占用校验（模块 3 遗留 TODO 已补回）：删分类时若子树内 `kb_article.category_id` 被未删知识引用，返回 `code=1001 分类下存在知识，无法删除`；删标签时若被 `kb_article_tag` 引用，返回 `code=1001 标签已被知识引用，无法删除`。

---

## 5. 检索 + 详情 + 互动（模块 5、6）

| 方法 | 路径 | 权限码 | 说明 |
|---|---|---|---|
| GET | `/search/article` | `knowledge:search` | 关键词（标题/摘要/正文全文）+ 分类/标签筛选，分页，**仅 ONLINE**（模块 5）✅ 已实现，见 §5.1 |
| GET | `/knowledge/article/{id}/view` | `knowledge:search` | 查看详情并埋点浏览（模块 5）✅ 已实现，见 §5.2 |
| POST | `/interaction/favorite`、DELETE `/interaction/favorite/{articleId}`、GET `/interaction/favorite` | `interaction:favorite` | 收藏 / 取消 / 我的收藏分页（模块 6）✅ 已实现，见 §5.3 |
| POST | `/interaction/like` | `interaction:like` | `{targetType(ARTICLE/COMMENT),targetId,type(1/-1)}` 赞/踩，返回最新互动态（模块 6）✅ 已实现 |
| POST | `/interaction/comment`、DELETE `/interaction/comment/{id}` | `interaction:comment` | 发表/回复评论、删除本人评论（模块 6）✅ 已实现 |
| GET | `/interaction/comment?articleId=` | `knowledge:search` | 某知识评论树（门户阅读者可见）（模块 6）✅ 已实现 |
| POST | `/interaction/share` | `interaction:share` | `{articleId,toUserId,message}` 分享给同事（即对收件人的站内通知）（模块 6）✅ 已实现 |
| GET | `/interaction/share/inbox`、`/interaction/share/sent` | `interaction:share` | 收到的分享（站内通知收件箱）/ 我发出的，分页（模块 6）✅ 已实现 |
| PUT | `/interaction/share/{id}/read` | `interaction:share` | 标记分享为已读（模块 6）✅ 已实现 |
| GET | `/interaction/share/unread-count` | `interaction:share` | 未读分享数（模块 6）✅ 已实现 |
| GET | `/interaction/users` | `interaction:share` | 可选收件人（启用用户 id+姓名），供坐席选人，避开 `system:user:list`（模块 6）✅ 已实现 |
| GET | `/interaction/state?articleId=` | `knowledge:search` | 某知识对当前用户的互动初态（收藏/我的点赞态/四项计数）（模块 6）✅ 已实现，见 §5.3 |

> 朗读（TTS）在客户端本地用 Qt TextToSpeech，无需后端接口。

### 5.1 知识检索（模块 5，已实现）

| 方法 | 路径 | 权限码 | 说明 |
|---|---|---|---|
| GET | `/search/article?page=&pageSize=&keyword=&categoryId=&tagId=` | `knowledge:search` | 分页（`PageResult<ArticleListItemVO>`，同 §4.3 列表项）。**仅返回 `status=ONLINE` 知识**；`keyword` 走 MySQL `ft_art_content`（ngram 全文，覆盖 title/summary/content，自带相关度排序）；`categoryId`/`tagId` 叠加筛选（**`categoryId` 含子分类**：先把所选分类展开为「自身 + 全部子孙分类」再 `IN` 过滤，故选父/根分类可搜全其下子分类的知识；`tagId` 先反查 `kb_article_tag`）；无 `keyword` 时退化为浏览式检索，按 `view_count desc, online_time desc` 排序。每次检索写一条 `kb_search_log`（含空关键词，记 `user_id/keyword/result_count/client_ip`）。 |

> 权限码 `knowledge:search` 已在 `V2__seed_rbac.sql`（菜单 201「知识检索」）种入并随 V2 末尾「ADMIN 授全部权限」覆盖，模块 5 **无需新增 Flyway 迁移**。检索/浏览均为 GET，不入 `sys_operation_log`（埋点分别落 `kb_search_log`/`kb_view_log`）。

### 5.2 知识详情查看 + 浏览埋点（模块 5，已实现）

| 方法 | 路径 | 权限码 | 说明 |
|---|---|---|---|
| GET | `/knowledge/article/{id}/view` | `knowledge:search` | 门户侧查看详情：返回 `ArticleDetailVO`（同 §4.3 详情，含富文本正文 + 标签[{id,name,color}] + 附件列表），同时原子 `view_count++` 并写一条 `kb_view_log`（`article_id/user_id/client_ip`）。`view_count` 自增用 `setSql` 空实体更新，**不触发审计字段自动填充**，故浏览不会改动 `update_time`（避免被浏览的知识在管理列表中被顶到最前）。返回体的 `viewCount` 已是自增后的值。 |

> 管理后台详情（`GET /knowledge/article/{id}`，权限 `knowledge:article:list`）不埋点、不自增，仅供管理/审核查看；门户阅读统一走 `/{id}/view`。

### 5.3 知识互动 + 站内通知（模块 6，已实现）

互动端点前缀 `/interaction`。计数列维护在 `kb_article`（`like_count/dislike_count/favorite_count/comment_count`，**无 share_count**）；自增镜像 §5.2 的 `setSql` 空实体更新（不触发审计填充、不动 `update_time`），自减用 `GREATEST(x-1,0)` 防负。当前用户取自 JWT。

| 方法 | 路径 | 权限码 | 说明 |
|---|---|---|---|
| POST | `/interaction/favorite` | `interaction:favorite` | 入参 `{articleId,folder?}`；按 `uk_fav_art_user`(article,user) 查重，**已收藏则幂等**（仅更新收藏夹，不重复计数），否则插入 + `favorite_count+1` |
| DELETE | `/interaction/favorite/{articleId}` | `interaction:favorite` | 取消收藏；删除生效时 `favorite_count = GREATEST(-1,0)` |
| GET | `/interaction/favorite?page=&pageSize=` | `interaction:favorite` | 我的收藏分页（`PageResult<FavoriteVO>`：`id/articleId/title/categoryName/knowledgeType/folder/createTime`），按收藏时间倒序 |
| POST | `/interaction/like` | `interaction:like` | 入参 `{targetType(ARTICLE/COMMENT),targetId,type(1赞/-1踩)}`；无既有表态→插入+计数；同 type 再点→取消（删除+计数-1）；异 type→改表态并双向调计数（ARTICLE 在 like/dislike 间搬移；COMMENT 仅 `like_count` 跟随 type=1）。返回最新 `InteractionStateVO` |
| POST | `/interaction/comment` | `interaction:comment` | 入参 `{articleId,parentId?(0/缺省为顶层),content}`；`parentId!=0` 校验父评论存在且同 article；插入（status NORMAL）+ `comment_count+1`；返回该条 `CommentVO` |
| DELETE | `/interaction/comment/{id}` | `interaction:comment` | **仅作者本人**（否则 `code=2003`）逻辑删（`@TableLogic`）+ `comment_count = GREATEST(-1,0)` |
| GET | `/interaction/comment?articleId=` | `knowledge:search` | 评论树（`List<CommentVO>`：顶层 + `children` 回复），仅 NORMAL 未删，按时间升序；富集 `userName`（realName 优先）。门户阅读者可见 |
| POST | `/interaction/share` | `interaction:share` | 入参 `{articleId,toUserId,message}`；校验 article 存在、收件人存在且 ENABLED；插入 `kb_share`（fromUser=当前、shareType `USER`、readStatus 0）。**该行即对收件人的站内通知** |
| GET | `/interaction/share/inbox?page=&pageSize=` | `interaction:share` | 收到的分享（站内通知收件箱），按 toUser 查、时间倒序（`PageResult<ShareVO>`：富集 articleTitle + 收发双方姓名） |
| GET | `/interaction/share/sent?page=&pageSize=` | `interaction:share` | 我发出的分享，按 fromUser 查、时间倒序 |
| PUT | `/interaction/share/{id}/read` | `interaction:share` | 标已读（`read_status=1`，限本人收件） |
| GET | `/interaction/share/unread-count` | `interaction:share` | 当前用户未读分享数（`read_status=0`） |
| GET | `/interaction/users` | `interaction:share` | 可选收件人（`List<UserOptionVO>`：`id/realName`），仅 ENABLED 且排除自己。坐席无 `system:user:list` 也能选人 |
| GET | `/interaction/state?articleId=` | `knowledge:search` | 某知识对当前用户的互动初态（`InteractionStateVO`：`favorited/myLikeType(1/-1/0)/likeCount/dislikeCount/favoriteCount/commentCount`），供详情弹窗加载与每次互动后刷新 |

> **站内通知 = 收到的分享**：本期不建独立通知表，「未读通知」即 `kb_share.read_status=0`，收件箱即 `/interaction/share/inbox`。
> **权限**：`interaction:favorite/share/comment` 已在 `V2__seed_rbac.sql` 种入并随 V2 末尾「ADMIN 授全权」覆盖；缺失的 `interaction:like`（点赞/点踩按钮，挂菜单 201 下，id 2012）由 **`V6__seed_module6_permissions.sql`** 幂等补入并授予 ADMIN（迁移链 v5→v6，不回改 V1–V5）。读端点 `/interaction/comment`、`/interaction/state` 用 `knowledge:search`，门户阅读者即可见。互动均不入 `sys_operation_log`（`/interaction/**` 不在拦截路径，与检索/浏览一致）。
> **注**：内置角色授权由 `V7__grant_builtin_roles.sql` 补齐——KNOWLEDGE_ADMIN/AUDITOR/AGENT/USER 此前在 `V2` 中**未被授予任何权限码**（仅 `ADMIN` 授全权），曾导致坐席等非 admin 账号访问受保护端点一律 403。V7 按职责幂等补授：知识管理员（分类·标签·知识 CRUD+附件+检索+互动）、审核员（审核+上下线+检索+互动）、坐席/普通用户（检索+收藏/赞踩/评论/分享）。模块 7 统计授权由 V8 单独补给 KNOWLEDGE_ADMIN/AUDITOR；模块 8 开放应用管理当前 V9 仅授 ADMIN，非 admin 授权按运营职责另行追加。

---

## 6. 数据统计 + 开放 API（模块 7、8）

| 方法 | 路径 | 权限码 | 说明 |
|---|---|---|---|
| GET | `/statistics/overview` | `statistics:view` | 仪表盘汇总（模块 7）✅ 已实现，见 §6.1 |
| GET | `/statistics/hot-article` | `statistics:view` | 热门知识（按累计浏览量，可选分类含子分类）（模块 7）✅ 已实现，见 §6.1 |
| GET | `/statistics/hot-keyword` | `statistics:view` | 热门搜索词（可选时间窗）（模块 7）✅ 已实现，见 §6.1 |

### 6.1 数据统计（模块 7，已实现）

统计端点前缀 `/statistics`，统一权限码 `statistics:view`（菜单 401「数据统计」，V2 已种入）。全部为只读 GET 聚合，不入 `sys_operation_log`。聚合分两类：标量计数/「今日」量走 MyBatis-Plus `selectCount`（自动应用逻辑删除），GROUP BY / SUM 走聚合 `StatisticsMapper` 的原生 `@Select`（**原生 SQL 绕过 `@TableLogic`，查 `kb_article` 处手写 `deleted = 0`**）。

| 方法 | 路径 | 权限码 | 说明 |
|---|---|---|---|
| GET | `/statistics/overview` | `statistics:view` | 仪表盘汇总，返回 `OverviewVO`（见下） |
| GET | `/statistics/hot-article?categoryId=&limit=` | `statistics:view` | 热门知识：按 `kb_article.view_count` 倒序的 **ONLINE** 知识 TOP（`limit` 默认 10、收敛 [1,50]）；`categoryId` 非空时展开为「自身 + 全部子孙分类」再 `IN` 过滤（**含子分类**，复用 `CategoryService.subtreeIds`）。返回 `List<HotArticleVO>` |
| GET | `/statistics/hot-keyword?days=&limit=` | `statistics:view` | 热门搜索词：`kb_search_log` 按 `keyword` 聚合检索次数倒序（`limit` 默认 10、收敛 [1,50]）；`days` 默认 30，`days>0` 限近 N 自然日（含今日）、`days<=0` 统计全部历史。返回 `List<HotKeywordVO>` |

`overview` 响应 `data`（`OverviewVO`）形态：

```json
{
  "articleTotal": 12, "articleOnline": 8, "articlePendingAudit": 1, "articleDraft": 2, "articleOffline": 1,
  "viewTotal": 1860, "likeTotal": 73, "favoriteTotal": 15, "commentTotal": 2,
  "categoryCount": 12, "tagCount": 8,
  "todayViews": 24, "todaySearches": 9, "todayNewArticles": 0,
  "statusDist": [ { "name": "ONLINE", "count": 8 }, { "name": "DRAFT", "count": 2 } ],
  "typeDist":   [ { "name": "SCRIPT", "count": 5 }, { "name": "PRODUCT", "count": 4 } ]
}
```

- `hot-article` 行（`HotArticleVO`）：`{id, title, categoryId, categoryName, knowledgeType, status, viewCount, likeCount, favoriteCount, commentCount}`。
- `hot-keyword` 行（`HotKeywordVO`）：`{keyword, searchCount, zeroCount}`，`zeroCount` 为该词命中 0 结果的检索次数（内容缺口信号）。

> **权限**：`statistics:view` 与菜单 401/目录 400 已在 `V2__seed_rbac.sql` 种入且 ADMIN 随 V2 末尾「ADMIN 授全权」覆盖。`V8__grant_statistics.sql` 幂等补**非 admin 角色**授权（沿 V7 遗留约定）：仅授后台分析角色 **KNOWLEDGE_ADMIN(2) / AUDITOR(3)**（含目录 400 作菜单父节点），坐席(4)/普通用户(5) 为门户消费侧不开放统计。`statistics:view` 无需新权限码，故 V8 仅授权、不新增权限。

### 6.2 开放 API（模块 8，已实现）

> 访问仍复用 Spring `context-path=/api`，因此完整 URL 为 `http://<host>:8080/api/open/v1/...`；下表路径均为 Base URL `http://<host>:8080/api` 之后的相对路径。开放端点由 `SecurityConfig` 放行 JWT，但必须通过 AppKey + HMAC 签名。

管理端（JWT + RBAC）：

| 方法 | 路径 | 权限码 | 说明 |
|---|---|---|---|
| GET | `/openapi/app?page=1&pageSize=20&keyword=&status=` | `openapi:app:list` | 开放应用分页，返回 `appSecretMasked`，不返回明文密钥 |
| GET | `/openapi/app/{id}` | `openapi:app:list` | 开放应用详情 |
| POST | `/openapi/app` | `openapi:app:create` | 新增开放应用；`appKey/appSecret` 可留空自动生成，创建响应仅本次返回明文 `appSecret` |
| PUT | `/openapi/app/{id}` | `openapi:app:update` | 编辑应用名称、状态、限流、scope、备注；`appSecret` 留空保持不变 |
| PUT | `/openapi/app/{id}/secret` | `openapi:app:update` | 重置 AppSecret，响应仅本次返回明文 |
| DELETE | `/openapi/app/{id}` | `openapi:app:delete` | 逻辑删除开放应用 |

对外端（AppKey + HMAC）：

| 方法 | 路径 | scope | 说明 |
|---|---|---|---|
| GET | `/open/v1/search?page=&pageSize=&keyword=&categoryId=&tagId=` | `search` | 对外检索，仅返回 ONLINE 知识列表，复用 `/search/article` 检索逻辑并写调用日志 |
| GET | `/open/v1/article/{id}` | `detail` | 对外详情，仅 ONLINE 可访问；不触发门户浏览量自增 |
| POST | `/open/v1/qa` | `qa` | 对外问答占位；二期 AI 未接入前返回 HTTP 501 + `code=6001` |

请求头：

| Header | 说明 |
|---|---|
| `X-App-Key` | 开放应用 `appKey` |
| `X-Timestamp` | Unix 秒级时间戳，允许服务端时间 ±300 秒 |
| `X-Nonce` | 调用方生成的随机串；同一应用 5 分钟窗口内不可重复 |
| `X-Signature` | `HmacSHA256` 十六进制小写签名 |

签名串：

```text
METHOD
PATH
CANONICAL_QUERY
SHA256(BODY)
X-Timestamp
X-Nonce
```

- `PATH` 为 Base URL 之后的应用路径，例如 `/open/v1/search`，不包含 Spring `context-path` 的 `/api`。
- `CANONICAL_QUERY` 按参数名升序、同名参数值升序拼接为 `key=value&key=value`；无查询参数时为空行。
- `BODY` 为原始请求体文本；GET 为空字符串，POST `/open/v1/qa` 使用原始 JSON 字符串。
- 应用 `scope` 以逗号分隔：`search,detail,qa`；每分钟限流由 `open_app.rate_limit` 控制；`X-Nonce` 在进程内按应用去重，过期窗口同时间戳允许偏差（5 分钟）；每次调用写入 `sys_api_log`。

权限与迁移：菜单 402「API 开放管理」和 `openapi:app:list` 已在 `V2__seed_rbac.sql` 种入，`V9__seed_module8_permissions.sql` 补 `openapi:app:create/update/delete` 并授 ADMIN；非 admin 角色授权按项目运营职责另行增量补授。

---

## 7. 健康检查 / 文档

- `GET /actuator/health` → 健康状态。
- Swagger UI：`/swagger-ui.html`（springdoc-openapi）。

---

## 8. 智能问答 + LLM 配置（模块 9，✅ 已实现）

> 模块 9 已全部落地（M9.0–M9.4）。Java 端端点前缀 `/ai`，权限码 `ai:qa`（人人可问）/ `ai:index`（管理员重建索引）/ `ai:config`（管理员 LLM 配置）。Python 内部端点前缀 `/ai`（FastAPI :8000），由 Java `AiClient`（RestClient）调用，Qt 客户端不直连。

### 8.1 问答（Java `/api/ai/qa`）

| 方法 | 路径 | 权限码 | 说明 |
|---|---|---|---|
| POST | `/ai/qa` | `ai:qa` | 提问。入参 `{question, topK?, categoryId?, sessionId?}`；出参 `{sessionId, messageId, answer, mode, citations:[{articleId,title,score,snippet}]}`。`mode` ∈ `llm`/`extractive`/`no_hit` |
| POST | `/ai/qa/feedback` | `ai:qa` | 反馈。入参 `{messageId, helpful(Int:1赞/0踩), reason?}` |
| GET | `/ai/qa/sessions` | `ai:qa` | 我的会话历史（`List<{id,title,createTime}>`） |
| GET | `/ai/qa/sessions/{id}/messages` | `ai:qa` | 某会话的消息记录 |

### 8.2 向量索引（Java `/api/ai/index`）

| 方法 | 路径 | 权限码 | 说明 |
|---|---|---|---|
| POST | `/ai/index/rebuild` | `ai:index` | 重建全部 ONLINE 知识索引。出参 `{total, indexed, failed, empty}` |
| POST | `/ai/index/rebuild/{articleId}` | `ai:index` | 重建单篇知识索引 |

> 上下线自动同步：`ArticleService` 审核 PASS / online / offline / delete 发送 `ArticleVectorSyncEvent`，`ArticleIndexListener`（`@TransactionalEventListener(AFTER_COMMIT)`）同步调用 Python 索引/删除，请求会等向量化完成。

### 8.3 LLM 运行时配置（Java `/api/ai/llm-config`）

| 方法 | 路径 | 权限码 | 说明 |
|---|---|---|---|
| GET | `/ai/llm-config` | `ai:config` | 查看当前配置。返回 `{baseUrl, model, temperature, maxTokens, enabled, configured, apiKeyMasked, pushWarning?}`。**apiKey 始终返回掩码**（`sk-t****WXYZ`），绝不回传明文 |
| PUT | `/ai/llm-config` | `ai:config` | 保存配置。入参 `{baseUrl, apiKey?, model, temperature, maxTokens, enabled}`。`apiKey` 留空=保持原值。Java 存 DB（权威源）+ best-effort 下发 Python；Python 不可用时仅存库，返回 `pushWarning` 告警 |
| POST | `/ai/llm-config/test` | `ai:config` | 测试连接。入参同 PUT（`apiKey` 留空则用库值）。Java 转发 Python 做一次极短 ping，返回 `{ok, message, latencyMs}`。**不落库、不改运行中单例** |

**配置流**：Qt → Java `PUT /api/ai/llm-config`（连字符）→ 存 `ai_llm_config`（DB 权威源）+ best-effort 下发 Python `POST /ai/llm/config`（斜杠）。Python 改 `settings.llm_*` + `llm.reset()` + 写 `runtime_llm_config.json`；启动时若该文件存在则覆盖 env 默认 → Python 自身重启不丢配置。`LlmConfigSyncRunner`@`ApplicationReadyEvent` 在 Java 启动时补发库中已启用配置。

### 8.4 Python 内部端点（FastAPI `:8000/ai/*`，Java 调用）

| 方法 | 路径 | 说明 |
|---|---|---|
| POST | `/ai/parse` | 解析附件文本。入参 `{article_id?, file_id?, file_path}` → `{chunks:[{index,text}]}` |
| POST | `/ai/embed` | 取原始向量。入参 `{texts:[string]}` → `{vectors:[[float]], dim:int}` |
| POST | `/ai/index` | 向量化并入库。入参 `{article_id, file_path?, texts?}` → `{article_id, chunk_count, dim}` |
| POST | `/ai/index/remove` | 删除某知识的全部向量块。入参 `{article_id}` → `{removed:int}` |
| POST | `/ai/qa` | RAG 问答（Java 调）。入参 `{question, top_k?, allowed_article_ids?}` → `{answer, mode, citations:[{article_id,title,score,snippet}]}` |
| GET | `/ai/llm/config` | 查看当前 LLM 配置状态（**无 api_key**） |
| POST | `/ai/llm/config` | 接收 Java 下发的 LLM 配置并即时生效 |
| POST | `/ai/llm/test` | 用候选配置建临时客户端做极短 ping（不改单例） |

---

## 9. 二期待实现（TODO）

- **实时辅助（模块 10）**：WebSocket `/ws/agent`（实时转写推送 + 推荐卡片）；CC/ASR 配置 `/realtime/config`。

> FastAPI 契约详见 `ai-service/app/schemas/ai.py`。
