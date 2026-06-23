# Qt 全站 UI 迁移总指南（AI 提示词 + 逐页规范）

> **目标**：让 sibling 项目的 Qt Widgets 客户端在视觉与交互上与 tuandui（智能客服知识库）一致。  
> **做法**：使用本文档 **§十二 完整 app.qss** + 按逐页清单改 layout / objectName。  
> **索引**：[`README.md`](README.md) · **首页/统计** [`首页与数据统计-迁移指南.md`](首页与数据统计-迁移指南.md) · **社区/个人中心** [`知识社区与个人中心-跨项目落地指南.md`](知识社区与个人中心-跨项目落地指南.md) · **API** [`API契约.md`](API契约.md)

---

## 一、给 AI 的总提示词（复制整段使用）

```markdown
# 任务：全站 Qt UI 迁移 — 对齐参考项目 tuandui 浅色 SaaS 风格

## 总目标
将整个客户端从「系统默认/表格堆叠/深色污染」迁移为：
- 页面底 #F4F6F9，白卡片 #FFFFFF，细边框 #E5E8EC，主色 #2563EB
- Fusion 样式 + 浅 QPalette + 全局 app.qss
- 全站 objectName 驱动样式，禁止业务代码随意 setStyleSheet

## 第一步：基础设施（必须先做）
1. 将本文档 **§十二** 完整 `app.qss` 保存为 `resources/styles/app.qss`（约 650 行，含问答页补充样式）
2. `main.cpp`：Fusion + applyLightPalette + 加载 `:/styles/app.qss`（见 §2.4）
3. `resources.qrc` 注册 qss
4. `MainWindow`：`ContentStack` 背景灰；`AppTopBar` → TopBar；`NavSidebar` → NavTree

## 第二步：识别页面类型并套用模板（见本文档 §四）
- A 仪表盘滚动页（dashboard）
- B 管理表格页（user/role/manage/favorite/share…）
- C 主从/分栏页（audit、user 机构树）
- D 社区信息流页（knowledge.search、profile）
- E 栈式列表↔详情（share、favorite、qa、search 详情）
- F SubTab 页（openapi、system.log、profile 发布/收藏）
- G 登录/占位/无权限（login、placeholder、access denied）
- H 对话页（qa）
- I 表单对话框（所有 *Dialog）

## 第三步：逐页迁移（按 §五 清单，每页对照「应有 objectName」改）
每改一页输出：原问题 → 改动点 → 验收截图描述

## 硬规则
| 规则 | 说明 |
|---|---|
| 按钮 | PrimaryButton / GhostButton 二分，禁止默认 QPushButton |
| 卡片 | 内容分区用 SectionCard，信息流用 FeedCard |
| 表格 | DataTable + 放 SectionCard 内 + 无 grid + 斑马纹 |
| 分页 | Ghost 上一页/下一页 + PageInfoLabel |
| 滚动 | QScrollArea setFrameShape(NoFrame)，背景 transparent |
| Tab | 个人中心等用 SubTabButton+StackedWidget；顶层 QTabWidget 可保留（分享收件/发件） |
| 间距 | 业务页 28,20,28,20 spacing 14；PageContainer 内 28,24,28,28 spacing 18 |
| 头像 | AvatarLabel 蓝底白字首字 |
| 标签 | TagStyle chip；筛选用 Primary/Ghost 按钮 |

## 禁止
- Win 深色主题下黑底黑字（必须 Fusion+Palette）
- 社区/个人中心用 QTableWidget 展示 feed
- 嵌套 QTabWidget 导致双层白框（个人中心）
- 整页 setBackgroundColor 破坏层次

## 交付
1. 完整 app.qss + main.cpp
2. §五 全部页面 migration 勾选表
3. §六 公共组件清单
4. 使用 §十二 完整 app.qss（已含 AnswerCard、CardTitle 等补充段）

请先扫描目标项目所有 *Page* / *Dialog*，按 §五 表格生成迁移计划，再逐页实施。
```

---

## 二、设计体系速查

### 2.1 色彩令牌

| 名称 | 色值 | 用途 |
|---|---|---|
| primary | `#2563EB` | 主按钮、选中、链接 |
| primary-light | `#EEF2FF` | 徽章、Tab hover |
| page-bg | `#F4F6F9` | MainWindow、ContentStack |
| surface | `#FFFFFF` | 卡片、输入、顶栏 |
| border | `#E5E8EC` | 卡片边框 |
| border-input | `#D8DCE2` | 输入框 |
| text-title | `#111827` | 大标题 |
| text-body | `#1F2A37` | 正文 |
| text-muted | `#6B7280` | 说明、统计 |
| text-hint | `#9098A3` | placeholder |
| select-bg | `#E8F0FE` | 表格/列表选中 |
| error | `#DC2626` | 错误 |
| welcome-grad | `#2563EB → #3B82F6` | 仪表盘横幅 |

### 2.2 字体与圆角

- 字体：`Microsoft YaHei UI`, `Segoe UI`, `PingFang SC`，正文 **14px**
- 圆角：按钮/输入 **8px**，卡片 **12px**，登录/占位 **14px**
- 顶栏高度 **56px**，侧栏项 **38px**

### 2.3 objectName 词典（全站）

| objectName | 用于 |
|---|---|
| **壳层** | TopBar, PageTitle, RoleBadge, ContentStack, PageContent, NavTree |
| **按钮** | PrimaryButton, GhostButton, SubTabButton, LinkButton |
| **卡片** | SectionCard, FeedCard, StatCard, LoginCard, PlaceholderCard, WelcomeBanner |
| **Feed** | FeedCardTitle, FeedCardStats |
| **Stat** | StatCardLabel, StatCardValue, StatCardHint |
| **表格** | DataTable |
| **Tab 区** | SubTabBar, SubTabPane |
| **文字** | PlaceholderTitle, SectionTitle, MutedLabel, HintLabel, PageInfoLabel, LoadingLabel, ErrorLabel, BrandTitle, BrandSubtitle, StatusLabel, WelcomeTitle, WelcomeHint |
| **内容** | ArticleContent, CommentItem |
| **占位** | ModuleChip, RouteChip, Phase2Chip, PendingChip, PlaceholderHint, PlaceholderMeta |
| **标签** | TagNameChip, TagHexChip |
| **问答** | AnswerCard, CardTitle, ProfileContentStack |
| **标签编辑** | TagColorPreview |

### 2.4 全局加载（main.cpp 模板）

```cpp
void applyLightPalette(QApplication &app) {
    QPalette pal;
    pal.setColor(QPalette::Window, QColor("#F4F6F9"));
    pal.setColor(QPalette::WindowText, QColor("#1F2A37"));
    pal.setColor(QPalette::Base, QColor("#FFFFFF"));
    pal.setColor(QPalette::AlternateBase, QColor("#FAFBFC"));
    pal.setColor(QPalette::Text, QColor("#1F2A37"));
    pal.setColor(QPalette::Button, QColor("#FFFFFF"));
    pal.setColor(QPalette::ButtonText, QColor("#1F2A37"));
    pal.setColor(QPalette::Highlight, QColor("#2563EB"));
    pal.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    pal.setColor(QPalette::PlaceholderText, QColor("#9098A3"));
    app.setPalette(pal);
}

// main():
QApplication::setStyle(QStyleFactory::create("Fusion"));
applyLightPalette(app);
QFile qss(":/styles/app.qss");
if (qss.open(QFile::ReadOnly | QFile::Text))
    app.setStyleSheet(QString::fromUtf8(qss.readAll()));
```

**resources.qrc 注册**：

```xml
<RCC>
    <qresource prefix="/">
        <file>styles/app.qss</file>
    </qresource>
</RCC>
```

### 2.5 app.qss 章节索引（对照 §十二）

| QSS 区块 | 行号区间（§十二） | 关联 objectName / 控件 |
|---|---|---|
| 全局基色 | 开头 | QWidget, ContentStack, QScrollArea |
| 顶栏 | `#TopBar` 段 | TopBar, PageTitle, RoleBadge, UserBadge |
| 侧栏 | `#NavTree` 段 | NavTree |
| 按钮 | `#PrimaryButton` / `#GhostButton` | PrimaryButton, GhostButton |
| 输入 | `QLineEdit` / `QComboBox` | 全站表单、筛选 |
| 多行/详情 | `QTextEdit` / `#ArticleContent` | 富文本、详情正文 |
| QTabWidget | `QTabWidget::pane` | share 收件/发件 |
| SubTab | `#SubTabButton` / `#SubTabPane` | openapi, system.log, profile |
| 列表 | `QListWidget` | 审核左栏、转发选人 |
| 表格 | `#DataTable` | DataTable |
| 登录 | `#LoginDialog` 段 | LoginCard, BrandTitle, LinkButton |
| 占位 | `#PlaceholderCard` 段 | PlaceholderPage, AccessDenied |
| 仪表盘 | `#WelcomeBanner` / `#StatCard` | dashboard |
| 卡片 | `#SectionCard` / `#FeedCard` | 全站分区、社区流 |
| 评论 | `#CommentItem` | ArticleDetailPanel |
| 滚动条 | `QScrollBar` | 全站 |
| **补充** | 文件末尾 | AnswerCard, CardTitle, ProfileContentStack, TagColorPreview |

> **完整源码见 §十二**，可直接整文件复制到 sibling 项目，无需再单独维护样式片段。

---

## 三、页面布局模板（8 种）

### 模板 A — 仪表盘滚动页

**用于**：`dashboard`

```
QVBoxLayout(0) → PageContainer#PageContent
  ├ WelcomeBanner → WelcomeTitle + WelcomeHint
  ├ LoadingLabel / ErrorLabel + GhostButton retry
  ├ QGridLayout → StatCard × N（4 列）
  └ SectionCard × N
       ├ SectionTitle
       └ DataTable / 自定义 body
```

**要点**：用 `PageContainer` 统一 margin；指标用 `StatCard` 组件；禁止裸 QLabel 做大数字。

---

### 模板 B — 管理表格页（最常用）

**用于**：`knowledge.manage`、`system.role`、`favorite`、`statistics` 部分区块

```
QVBoxLayout(28,20,28,20) spacing 14
  ├ SectionCard（工具栏）
  │    ├ QLineEdit / QComboBox 筛选
  │    ├ PrimaryButton「查询/搜索」
  │    └ GhostButton「新增/刷新/删除…」
  ├ QTableWidget#DataTable
  │    setShowGrid(false); setAlternatingRowColors(true);
  │    verticalHeader invisible; SelectRows
  └ QHBoxLayout: Ghost 上一页 | Ghost 下一页 | PageInfoLabel | stretch
```

**要点**：表格不单独再套 heavy border（DataTable 在 SectionCard 外时保留圆角边框）。

---

### 模板 C — 左树/列表 + 右详情

**用于**：`system.user`（机构树+人员表）、`audit`（待审列表+预览）

```
QVBoxLayout(28,20) → QSplitter  horizontal
  ├ 左：NavTree 或 QListWidget#DataTable + 小标题
  └ 右：SectionCard
         ├ SectionTitle
         ├ ArticleContent / DataTable
         └ Primary + Ghost 操作按钮
```

**要点**：右侧必须是 SectionCard；审核「通过」Primary，「驳回」Ghost。

---

### 模板 D — 知识社区 / 信息流

**用于**：`knowledge.search`、`UserProfilePanel` 内列表

```
QVBoxLayout(28,20) 或 QStackedWidget#ContentStack
  ├ 顶行：PlaceholderTitle + stretch + QLineEdit(搜索)
  ├ Primary/Ghost 排序按钮组（切换时 polish）
  ├ SectionCard：常用标签 + Ghost 编辑 + 标签按钮行
  ├ QScrollArea(frameless) → FeedCard × N
  └ MutedLabel 加载状态
```

**分页**：offset 0/15 + 后续 offset+=10；scroll 近底 loadMore。

---

### 模板 E — 栈式 列表 ↔ 详情

**用于**：`share`、`favorite`、`knowledge.search`（含 profile）、`qa`

```
QVBoxLayout(0) → QStackedWidget#ContentStack
  ├ listPage（模板 B 或 D）
  └ ArticleDetailPanel / UserProfilePanel
```

**返回逻辑**：share/favorite → 回列表；search 内 profile → 回详情。

---

### 模板 F — SubTab 切换

**用于**：`openapi`、`system.log`、`UserProfilePanel`

```
SectionCard(margin 0)
  ├ QHBoxLayout#SubTabBar → SubTabButton × N（checkable, exclusive）
  └ QStackedWidget 或 #SubTabPane
       └ 各 tab 内容（ often 模板 B）
```

**禁止**：SubTab 下再套 QTabWidget。

---

### 模板 G — 登录 / 占位 / 无权限

**LoginDialog**：`#LoginDialog` 灰底 → `#LoginCard` 白卡片居中 → BrandTitle + 输入 + Primary 登录 + HintLabel

**PlaceholderPage**：PageContainer 居中 `#PlaceholderCard` + ModuleChip + PlaceholderTitle + RouteChip

**AccessDeniedPage**：同上简化版 + RouteChip

---

### 模板 H — 智能问答

**用于**：`qa`

```
QVBoxLayout(28,20) → QStackedWidget
  ├ chatPage
  │    ├ MutedLabel 说明
  │    ├ QScrollArea → 气泡 SectionCard(用户) / AnswerCard(AI)
  │    ├ SectionCard 输入条 + Primary 提问
  │    └ SectionCard 引用列表 + Ghost 有用/无用/重建索引
  └ ArticleDetailPanel
```

---

### 模板 I — 对话框

**用于**：所有 `*Dialog*`

```
QVBoxLayout
  ├ HintLabel（可选）
  ├ SectionCard → QFormLayout 表单
  └ stretch + Ghost 取消 + Primary 确定
```

QDialogButtonBox 的 OK/Cancel 同样设 Primary/Ghost。

---

## 四、逐页迁移清单

> **图例**：模板 = §三 模板字母；优先级 P0 必改 / P1 建议 / P2 细调

### 4.1 应用壳层

| 页面/组件 | route/文件 | 模板 | 迁移要点 | 优先级 |
|---|---|---|---|---|
| 登录 | `LoginDialog` | G | LoginDialog 灰底；LoginCard；BrandTitle；Primary 登录；LinkButton 服务器设置 | P0 |
| 主窗口 | `MainWindow` | — | ContentStack；TopBar 56px；NavTree 侧栏；用户名 Ghost 可点 | P0 |
| 顶栏 | `AppTopBar` | — | PageTitle 17px；RoleBadge 蓝 pill；Ghost 退出 | P0 |
| 侧栏 | `NavSidebar` | — | NavTree 选中 #E8F0FE + 蓝字 | P0 |
| 无权限 | `AccessDeniedPage` | G | PlaceholderTitle + PlaceholderHint + RouteChip | P1 |
| 占位/二期 | `PlaceholderPage` | G | PlaceholderCard 居中；Phase2Chip 黄色 | P1 |
| 滚动容器 | `PageContainer` | A | frameless scroll；PageContent margin 28,24,28,28 | P0 |

---

### 4.2 工作台

| 页面 | route | 模板 | 迁移要点 | 优先级 |
|---|---|---|---|---|
| 首页仪表盘 | `dashboard` | A | WelcomeBanner 蓝渐变；StatCard 网格；SectionCard+DataTable 热门 Top5；Loading/Error/重试 | P0 |

**StatCard 用法**：`StatCard("标签", parent)` → setValue / setHint

---

### 4.3 知识应用

| 页面 | route | 模板 | 迁移要点 | 优先级 |
|---|---|---|---|---|
| 知识检索/社区 | `knowledge.search` | D+E | 见《知识社区指南》；FeedCard；PinnedTags；offset 分页；作者进 profile | P0 |
| 知识详情 | `ArticleDetailPanel` | — | PlaceholderTitle；Avatar 36 + Ghost 作者名；ArticleContent；SectionCard 评论；Ghost 赞/藏/分享 | P0 |
| 个人中心 | `personal.center` | F+D+E | ProfilePage stacked；UserProfilePanel；SubTab 发布/收藏；隐私 checkbox | P0 |
| 我的收藏 | `favorite` | B+E | SectionCard 工具栏；DataTable；点行进详情；Ghost 取消收藏 | P1 |
| 我的分享 | `share` | B+E | QTabWidget 收件/发件（可保留）；Phase2Chip 未读；DataTable；进详情 | P1 |
| 信息流卡片 | `ArticleFeedCard` | — | FeedCard + hover + 整卡可点 | P0 |
| 常用标签编辑 | `PinTagsEditDialog` | I | 多选列表 + Primary 保存 | P1 |
| 改密 | `ChangePasswordDialog` | I | 表单 + Primary 确认 | P1 |
| 新增知识 | `ArticleCreateDialog` | I | SectionCard 表单 | P1 |
| 转发选人 | `ShareUserDialog` | I | DataTable 用户 + Primary 转发 | P2 |

---

### 4.4 知识管理

| 页面 | route | 模板 | 迁移要点 | 优先级 |
|---|---|---|---|---|
| 知识管理 | `knowledge.manage` | B | SectionCard 筛选+Primary 查询+Ghost 新增；DataTable 状态列；行内 Ghost 操作 | P0 |
| 审核中心 | `audit` | C | 左待审列表；右 SectionCard+ArticleContent；Primary 通过 / Ghost 驳回 | P0 |
| 分类标签 | `category` | B | QTabWidget 分类树/标签表；tag 表用 TagChip delegate；TagFormDialog 用 TagStyle 预览 | P1 |
| 文章表单 | `ArticleFormDialog` | I | 大表单 SectionCard；Primary 提交 | P1 |

---

### 4.5 统计与开放

| 页面 | route | 模板 | 迁移要点 | 优先级 |
|---|---|---|---|---|
| 数据统计 | `statistics` | A+B | 多 SectionCard；SectionTitle；DataTable 热门/零结果词；Loading/Error | P1 |
| API 开放 | `openapi` | F+B | SubTab 应用管理/调用日志；SectionCard 筛选；DataTable；HintLabel | P1 |
| 开放应用表单 | `OpenAppFormDialog` | I | SectionCard + HintLabel | P2 |

---

### 4.6 智能与实时

| 页面 | route | 模板 | 迁移要点 | 优先级 |
|---|---|---|---|---|
| 知识问答 | `qa` | H+E | 气泡 SectionCard/AnswerCard；Primary 提问；引用列表；进详情 | P1 |
| 坐席辅助 | `agent` | G | 二期 PlaceholderPage 即可 | P2 |
| CC 配置 | `cc.config` | G | 同上 | P2 |

---

### 4.7 系统管理

| 页面 | route | 模板 | 迁移要点 | 优先级 |
|---|---|---|---|---|
| 机构人员 | `system.user` | C+B | 左 NavTree 机构；右 SectionCard 工具栏+DataTable+分页；UserDialogs | P0 |
| 角色管理 | `system.role` | B | 标准表格页；RoleDialogs 权限树用 NavTree | P0 |
| 菜单权限 | `system.permission` | C | 权限树 DataTable/Tree + Ghost CRUD | P1 |
| 系统日志 | `system.log` | F+B | SubTab 操作/登录；SubTabPane 内 SectionCard 筛选+DataTable | P1 |
| 用户/角色/权限对话框 | `*Dialogs*` | I | 统一 Primary/Ghost | P1 |

---

## 五、公共组件规范

| 组件 | 路径 | 规范 |
|---|---|---|
| AvatarLabel | `common/AvatarLabel.*` | 圆 `#2563EB`，首字，size 36/56 |
| TagStyle | `common/TagStyle.*` | createTagNameChip；半透明底色 |
| StatCard | `common/widgets/StatCard.*` | 仪表盘指标，objectName StatCard* |
| PageContainer | `common/widgets/PageContainer.*` | 带标准 margin 的滚动页容器 |
| AppTopBar | `common/widgets/AppTopBar.*` | TopBar 壳 |
| NavSidebar | `common/widgets/NavSidebar.*` | NavTree |
| Notify | `core/notify/Notify.*` | warn/error/info 统一提示 |
| ApiClient | `core/network/ApiClient.*` | 不在 UI 拼 HTTP |

---

## 六、编码惯例（全站）

### 6.1 页面基类

- 菜单页继承 `RefreshablePage`，实现 `refreshPage()`
- `PageRouter` 懒加载 factory 注册 route（与后端 menu.name 一致）

### 6.2 异步

```cpp
QPointer<MyPage> self(this);
api.call([self](...) { if (!self) return; ... });
```

### 6.3 排序/标签按钮切换

```cpp
btn->setObjectName(selected ? "PrimaryButton" : "GhostButton");
btn->style()->unpolish(btn);
btn->style()->polish(btn);
```

### 6.4 SubTab 工厂

```cpp
QPushButton *makeSubTabButton(const QString &text, QWidget *parent) {
    auto *b = new QPushButton(text, parent);
    b->setObjectName("SubTabButton");
    b->setCheckable(true);
    b->setAutoExclusive(true);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}
```

### 6.5 表格页工厂

```cpp
void setupDataTable(QTableWidget *t) {
    t->setObjectName("DataTable");
    t->verticalHeader()->setVisible(false);
    t->setShowGrid(false);
    t->setAlternatingRowColors(true);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->horizontalHeader()->setStretchLastSection(true);
}
```

### 6.6 CMake

每新增 `.h/.cpp` 写入 `CMakeLists.txt`；include 用工程根相对路径。

---

## 七、迁移执行顺序（推荐）

```
1. main.cpp + app.qss + qrc                    → 全局生效
2. MainWindow / TopBar / NavSidebar              → 壳层
3. LoginDialog                                   → 第一印象
4. dashboard                                     → 模板 A 验证
5. knowledge.search + FeedCard + Detail          → 模板 D（核心）
6. personal.center + UserProfilePanel            → 模板 F+D
7. knowledge.manage + audit                      → 模板 B/C
8. system.user + system.role                     → 模板 B/C
9. favorite + share + statistics + openapi       → 模板 B/E/F
10. qa + 对话框 + 其余                            → 模板 H/I
11. Placeholder / AccessDenied                   → 模板 G
12. 全站走查 §八 验收清单
```

---

## 八、全站验收清单

### 8.1 全局

- [ ] Win11 深色模式下输入框、下拉框文字可读
- [ ] ContentStack 灰底、卡片白底，层次分明
- [ ] 无主色以外的系统绿/灰按钮混入
- [ ] 滚动条细圆角（app.qss 已定义）
- [ ] 侧栏选中蓝色高亮

### 8.2 分页面

- [ ] 登录：LoginCard 居中，Primary 蓝按钮
- [ ] 仪表盘：WelcomeBanner 渐变 + StatCard 网格
- [ ] 知识社区：FeedCard hover，15+10 分页无重复
- [ ] 详情：作者 Avatar + 可点进主页
- [ ] 个人中心：SubTab 无双层白框；收藏 Tab 他人可见
- [ ] 管理页：SectionCard 工具栏 + DataTable 斑马纹
- [ ] 审核：左列表右 SectionCard 预览
- [ ] 机构人员：左 NavTree 右表格
- [ ] 开放 API：SubTab + 双表格
- [ ] 问答：用户/AI 气泡区分 SectionCard vs AnswerCard
- [ ] 对话框：全部 Primary/Ghost 底栏

### 8.3 反模式扫描

```bash
# 在目标项目执行：找出需清理的 inline 样式
rg "setStyleSheet" knowledgeAnswer/ --glob "*.cpp"
# 仅应出现在：AvatarLabel、TagStyle、CategoryTag 颜色预览、极少数富文本
```

---

## 九、与 sibling 项目的差异适配

在总提示词末尾附加（示例）：

```markdown
## 本项目差异
- 客户端目录：__________
- namespace：__________
- route 命名是否与参考一致：是/否（若否列出对照）
- 权限码前缀：__________
- 是否已有 app.qss：是/否
- 优先迁移页面（可只要 P0）：__________
```

---

## 十、参考文件一键复制清单

| 源路径（tuandui） | 说明 |
|---|---|
| `knowledgeAnswer/resources/styles/app.qss` | 全局 QSS（**本文 §十二 已内嵌完整内容**） |
| `knowledgeAnswer/main.cpp` | Fusion+Palette 加载 |
| `knowledgeAnswer/resources.qrc` | 资源注册 |
| `knowledgeAnswer/common/AvatarLabel.*` | 头像 |
| `knowledgeAnswer/common/TagStyle.*` | 标签 chip |
| `knowledgeAnswer/common/widgets/StatCard.*` | 指标卡 |
| `knowledgeAnswer/common/widgets/PageContainer.*` | 滚动容器 |
| `knowledgeAnswer/common/widgets/AppTopBar.*` | 顶栏 |
| `knowledgeAnswer/common/widgets/NavSidebar.*` | 侧栏 |
| `knowledgeAnswer/modules/knowledge/search/ArticleFeedCard.*` | 信息流卡片 |

---

## 十一、相关文档

| 文档 | 内容 |
|---|---|
| [`README.md`](README.md) | 迁移文档总索引 |
| [`首页与数据统计-迁移指南.md`](首页与数据统计-迁移指南.md) | dashboard + statistics 专项 |
| [`知识社区与个人中心-跨项目落地指南.md`](知识社区与个人中心-跨项目落地指南.md) | 功能、API、分页、后端 Flyway |
| [`API契约.md`](API契约.md) | REST 接口详约 |
| [`Qt前端样式规范与迁移提示词.md`](Qt前端样式规范与迁移提示词.md) | 样式专项（本文档已合并扩展，以此总指南为准） |

---

## 十二、完整 app.qss（可直接复制）

> 源文件：`knowledgeAnswer/resources/styles/app.qss`  
> 用法：保存为 `resources/styles/app.qss`，在 `resources.qrc` 注册为 `:/styles/app.qss`，由 `main.cpp` 加载（§2.4）。  
> 末尾 **补充段** 为问答页 / 个人中心栈 / 标签色预览，仓库 qss 若尚未包含可自行追加或以此文档为准。

```css
/* ============================================================
   智能客服知识库系统 — 客户端主题（浅色高对比）
   白底 + 深色文字 + #2563EB 主色按钮 + #E5E8EC 边框划分区域
   ============================================================ */

* {
    font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI", "PingFang SC", sans-serif;
}

/* ----- 全局基色：避免系统深色主题污染 ----- */
QWidget {
    background-color: #FFFFFF;
    color: #1F2A37;
    font-size: 14px;
}

QMainWindow,
#ContentStack,
QStackedWidget {
    background-color: #F4F6F9;
}

QDialog {
    background-color: #FFFFFF;
    color: #1F2A37;
}

QScrollArea {
    background-color: transparent;
    border: none;
}

QScrollArea > QWidget > QWidget {
    background-color: transparent;
}

/* ===== 顶栏 ===== */
#TopBar {
    background-color: #FFFFFF;
    border-bottom: 1px solid #E5E8EC;
}
#PageTitle {
    font-size: 17px;
    font-weight: 600;
    color: #111827;
    background: transparent;
}
#UserBadge {
    color: #6B7280;
    font-size: 13px;
    background: transparent;
}
#RoleBadge {
    background-color: #EEF2FF;
    color: #2563EB;
    border-radius: 6px;
    padding: 4px 10px;
    font-size: 12px;
    font-weight: 600;
}

/* ===== 侧边导航 ===== */
#NavTree {
    background-color: #FFFFFF;
    color: #374151;
    border: none;
    border-right: 1px solid #E5E8EC;
    outline: 0;
    padding: 8px 6px;
}
#NavTree::item {
    height: 38px;
    border-radius: 8px;
    padding-left: 12px;
    margin: 2px 4px;
    color: #374151;
    background-color: transparent;
}
#NavTree::item:hover {
    background-color: #F0F3F8;
}
#NavTree::item:selected {
    background-color: #E8F0FE;
    color: #2563EB;
    font-weight: 600;
}

/* ===== 按钮 ===== */
QPushButton#PrimaryButton {
    background-color: #2563EB;
    color: #FFFFFF;
    border: none;
    border-radius: 8px;
    padding: 9px 18px;
    font-size: 14px;
    font-weight: 600;
}
QPushButton#PrimaryButton:hover { background-color: #1D4FD7; }
QPushButton#PrimaryButton:pressed { background-color: #1A45BE; }
QPushButton#PrimaryButton:disabled {
    background-color: #A9C2F5;
    color: #FFFFFF;
}

QPushButton#GhostButton {
    background-color: #FFFFFF;
    color: #374151;
    border: 1px solid #D8DCE2;
    border-radius: 8px;
    padding: 7px 14px;
}
QPushButton#GhostButton:hover {
    color: #2563EB;
    border-color: #2563EB;
    background-color: #F8FAFF;
}
QPushButton#GhostButton:pressed {
    background-color: #EEF2FF;
}

/* ===== 单行输入 ===== */
QLineEdit {
    background-color: #FFFFFF;
    color: #1F2A37;
    border: 1px solid #D8DCE2;
    border-radius: 8px;
    padding: 9px 12px;
    selection-background-color: #2563EB;
    selection-color: #FFFFFF;
}
QLineEdit:focus {
    border: 1px solid #2563EB;
}
QLineEdit:disabled {
    background-color: #F2F4F7;
    color: #9098A3;
}

/* ===== 下拉框（标签筛选等）===== */
QComboBox {
    background-color: #FFFFFF;
    color: #1F2A37;
    border: 1px solid #D8DCE2;
    border-radius: 8px;
    padding: 8px 12px;
    min-height: 22px;
}
QComboBox:hover {
    border-color: #2563EB;
}
QComboBox:focus {
    border-color: #2563EB;
}
QComboBox:disabled {
    background-color: #F2F4F7;
    color: #9098A3;
}
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 28px;
    border: none;
    border-left: 1px solid #E5E8EC;
    border-top-right-radius: 8px;
    border-bottom-right-radius: 8px;
    background-color: #F8FAFC;
}
QComboBox::drop-down:hover {
    background-color: #EEF2FF;
}
QComboBox::down-arrow {
    width: 0;
    height: 0;
    border-left: 5px solid transparent;
    border-right: 5px solid transparent;
    border-top: 6px solid #6B7280;
}
QComboBox QAbstractItemView {
    background-color: #FFFFFF;
    color: #1F2A37;
    border: 1px solid #E5E8EC;
    border-radius: 8px;
    padding: 4px;
    outline: 0;
    selection-background-color: #E8F0FE;
    selection-color: #111827;
}
QComboBox QAbstractItemView::item {
    min-height: 32px;
    padding: 6px 12px;
    color: #1F2A37;
    background-color: #FFFFFF;
}
QComboBox QAbstractItemView::item:hover {
    background-color: #F0F3F8;
    color: #111827;
}
QComboBox QAbstractItemView::item:selected {
    background-color: #E8F0FE;
    color: #2563EB;
}

/* ===== 多行文本 / 富文本详情 ===== */
QTextEdit,
QPlainTextEdit {
    background-color: #FFFFFF;
    color: #1F2A37;
    border: 1px solid #D8DCE2;
    border-radius: 8px;
    padding: 10px 12px;
    selection-background-color: #2563EB;
    selection-color: #FFFFFF;
}
QTextEdit:focus,
QPlainTextEdit:focus {
    border-color: #2563EB;
}

QTextBrowser,
QTextBrowser#ArticleContent {
    background-color: #FFFFFF;
    color: #1F2A37;
    border: 1px solid #E5E8EC;
    border-radius: 10px;
    padding: 14px 16px;
    selection-background-color: #2563EB;
    selection-color: #FFFFFF;
}

/* ===== 选项卡（我的分享等）===== */
QTabWidget::pane {
    border: 1px solid #E5E8EC;
    border-radius: 0 0 10px 10px;
    background-color: #FFFFFF;
    top: -1px;
}
QTabBar::tab {
    background-color: #F4F6F9;
    color: #6B7280;
    border: 1px solid #E5E8EC;
    border-bottom: none;
    padding: 10px 22px;
    margin-right: 4px;
    border-top-left-radius: 8px;
    border-top-right-radius: 8px;
}
QTabBar::tab:selected {
    background-color: #FFFFFF;
    color: #2563EB;
    font-weight: 600;
    border-bottom: 2px solid #2563EB;
}
QTabBar::tab:hover:!selected {
    background-color: #EEF2FF;
    color: #374151;
}

/* ===== 子页面 Tab 切换（API 开放管理等）===== */
#SubTabBar {
    background: transparent;
}
QPushButton#SubTabButton {
    background-color: #F4F6F9;
    color: #6B7280;
    border: 1px solid #E5E8EC;
    border-bottom: none;
    padding: 10px 22px;
    border-top-left-radius: 8px;
    border-top-right-radius: 8px;
    min-width: 96px;
}
QPushButton#SubTabButton:hover {
    background-color: #EEF2FF;
    color: #374151;
}
QPushButton#SubTabButton:checked {
    background-color: #FFFFFF;
    color: #2563EB;
    font-weight: 600;
    border-bottom: 2px solid #2563EB;
}
#SubTabPane {
    background-color: #FFFFFF;
    border: 1px solid #E5E8EC;
    border-radius: 0 0 10px 10px;
}

/* ===== 列表（转发选人等）===== */
QListWidget {
    background-color: #FFFFFF;
    color: #1F2A37;
    border: 1px solid #E5E8EC;
    border-radius: 8px;
    outline: 0;
}
QListWidget::item {
    padding: 10px 12px;
    border-bottom: 1px solid #F0F3F8;
    color: #1F2A37;
    background-color: #FFFFFF;
}
QListWidget::item:hover {
    background-color: #F8FAFC;
}
QListWidget::item:selected {
    background-color: #E8F0FE;
    color: #111827;
}

/* ===== 表格 ===== */
QTableWidget,
QTableView {
    background-color: #FFFFFF;
    color: #1F2A37;
    border: 1px solid #E5E8EC;
    border-radius: 8px;
    gridline-color: #EEF2F7;
    selection-background-color: #E8F0FE;
    selection-color: #111827;
    alternate-background-color: #FAFBFC;
    outline: 0;
}
QTableWidget#DataTable {
    border: none;
    border-radius: 0;
}
QHeaderView::section {
    background-color: #F8FAFC;
    color: #374151;
    font-size: 13px;
    font-weight: 600;
    border: none;
    border-bottom: 1px solid #E5E8EC;
    border-right: 1px solid #F0F3F8;
    padding: 10px 8px;
}
QTableWidget::item,
QTableView::item {
    padding: 8px;
    color: #1F2A37;
    background-color: #FFFFFF;
}
QTableWidget::item:alternate,
QTableView::item:alternate {
    background-color: #FAFBFC;
}
QTableWidget::item:selected,
QTableView::item:selected {
    background-color: #E8F0FE;
    color: #111827;
}

/* ===== 复选框 ===== */
QCheckBox {
    color: #374151;
    spacing: 6px;
    background: transparent;
}

/* ===== 登录页 ===== */
#LoginDialog {
    background-color: #F4F6F9;
}
#LoginCard {
    background-color: #FFFFFF;
    border: 1px solid #E5E8EC;
    border-radius: 14px;
}
#BrandTitle {
    font-size: 22px;
    font-weight: 700;
    color: #111827;
    background: transparent;
}
#BrandSubtitle {
    font-size: 13px;
    color: #9098A3;
    background: transparent;
}
#StatusLabel {
    color: #DC2626;
    font-size: 13px;
    background: transparent;
}
#HintLabel {
    color: #9098A3;
    font-size: 12px;
    background: transparent;
}
#PageInfoLabel {
    color: #6B7280;
    font-size: 13px;
    background: transparent;
    padding-left: 8px;
}
QToolButton#LinkButton {
    background: transparent;
    border: none;
    color: #9098A3;
    padding: 2px 0;
}
QToolButton#LinkButton:hover {
    color: #2563EB;
}

/* ===== 占位页 ===== */
#PlaceholderCard {
    background-color: #FFFFFF;
    border: 1px solid #E5E8EC;
    border-radius: 14px;
}
#ModuleChip {
    color: #2563EB;
    font-size: 13px;
    font-weight: 600;
    background: transparent;
}
#PlaceholderMeta {
    background-color: #F8FAFC;
    border: 1px solid #EEF2F7;
    border-radius: 10px;
    color: #4B5563;
    font-size: 13px;
}
#PlaceholderMeta QLabel {
    color: #4B5563;
    background: transparent;
}
#PendingChip {
    background-color: #F3F4F6;
    color: #6B7280;
    border-radius: 6px;
    padding: 5px 12px;
    font-size: 12px;
}
#PlaceholderTitle {
    font-size: 24px;
    font-weight: 700;
    color: #111827;
    background: transparent;
}
#PlaceholderHint {
    font-size: 14px;
    color: #6B7280;
    background: transparent;
}
#RouteChip {
    background-color: #EEF2F7;
    color: #6B7280;
    border-radius: 6px;
    padding: 5px 12px;
    font-size: 12px;
}
#Phase2Chip {
    background-color: #FEF3C7;
    color: #B45309;
    border-radius: 6px;
    padding: 5px 12px;
    font-size: 12px;
    font-weight: 600;
}

/* ===== 页面内容区 ===== */
#PageContent {
    background-color: transparent;
}

/* ===== 欢迎横幅 ===== */
#WelcomeBanner {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #2563EB, stop:1 #3B82F6);
    border-radius: 12px;
}
#WelcomeTitle {
    font-size: 22px;
    font-weight: 700;
    color: #FFFFFF;
    background: transparent;
}
#WelcomeHint {
    font-size: 14px;
    color: #FFFFFF;
    background: transparent;
}

/* ===== 指标卡片 ===== */
#StatCard {
    background-color: #FFFFFF;
    border: 1px solid #E5E8EC;
    border-radius: 12px;
}
#StatCardLabel {
    font-size: 13px;
    color: #6B7280;
    background: transparent;
}
#StatCardValue {
    font-size: 28px;
    font-weight: 700;
    color: #111827;
    background: transparent;
}
#StatCardHint {
    font-size: 12px;
    color: #9098A3;
    background: transparent;
}

/* ===== 分区卡片 ===== */
#SectionCard {
    background-color: #FFFFFF;
    border: 1px solid #E5E8EC;
    border-radius: 12px;
}
#SectionCard QLabel {
    background: transparent;
}

/* ===== 知识社区流卡片 ===== */
#FeedCard {
    background-color: #FFFFFF;
    border: 1px solid #E5E8EC;
    border-radius: 12px;
}
#FeedCard:hover {
    border-color: #93C5FD;
    background-color: #F8FAFC;
}
#FeedCardTitle {
    font-size: 16px;
    font-weight: 600;
    color: #111827;
    background: transparent;
}
#FeedCardStats {
    font-size: 12px;
    color: #6B7280;
    background: transparent;
}

#SectionTitle {
    font-size: 16px;
    font-weight: 600;
    color: #111827;
    background: transparent;
}
#LoadingLabel {
    color: #6B7280;
    font-size: 14px;
    background: transparent;
}
#ErrorLabel {
    color: #DC2626;
    font-size: 13px;
    background: transparent;
}

#ArticleContent {
    background-color: #FFFFFF;
    color: #1F2A37;
    border: 1px solid #E5E8EC;
    border-radius: 10px;
}

#CommentItem {
    background-color: #FAFBFC;
    border: 1px solid #E5E8EC;
    border-radius: 8px;
}
#CommentItem QLabel {
    background: transparent;
    color: #1F2A37;
}

/* ===== 表单标签 ===== */
QLabel {
    background: transparent;
    color: #374151;
}

/* ===== 滚动条 ===== */
QScrollBar:vertical {
    background: transparent;
    width: 10px;
    margin: 2px;
}
QScrollBar::handle:vertical {
    background: #CBD2DA;
    border-radius: 5px;
    min-height: 30px;
}
QScrollBar::handle:vertical:hover {
    background: #AEB7C2;
}
QScrollBar:horizontal {
    background: transparent;
    height: 10px;
    margin: 2px;
}
QScrollBar::handle:horizontal {
    background: #CBD2DA;
    border-radius: 5px;
    min-width: 30px;
}
QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical,
QScrollBar::add-line:horizontal,
QScrollBar::sub-line:horizontal {
    width: 0;
    height: 0;
}
QScrollBar::add-page:vertical,
QScrollBar::sub-page:vertical,
QScrollBar::add-page:horizontal,
QScrollBar::sub-page:horizontal {
    background: transparent;
}

/* ===== 补充：问答页 / 个人中心栈 / 标签色预览 ===== */
#AnswerCard {
    background-color: #F8FAFC;
    border: 1px solid #E5E8EC;
    border-radius: 12px;
}
#CardTitle {
    font-size: 12px;
    font-weight: 600;
    color: #6B7280;
    background: transparent;
}
#ProfileContentStack {
    background: transparent;
}
#TagColorPreview {
    border-radius: 6px;
}
```

---

*版本：2026-06-23 · 覆盖 PageCatalog 全部 17 个业务 route + 壳层/对话框/公共组件 + 完整 app.qss*
