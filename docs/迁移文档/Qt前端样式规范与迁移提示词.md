# Qt 前端样式规范与迁移提示词

> 来源：tuandui 项目 `knowledgeAnswer/resources/styles/app.qss` 及全站 UI 约定。  
> 用途：在 sibling 项目中 **统一替换旧样式**，或作为 AI 提示词让界面达到当前项目水准。

---

## 1. 给 AI 的样式迁移提示词（可直接复制）

```markdown
# 任务：将 Qt Widgets 客户端 UI 统一为「浅色高对比 · 蓝主色」设计体系

## 目标观感
- 整体：浅灰页面底 `#F4F6F9` + 白卡片 + 细灰边框，类似现代 SaaS 后台（非 Win32 原生灰、非深色主题）
- 主色：`#2563EB`（按钮、选中态、链接 hover）
- 禁止：系统默认深色主题污染、纯表格堆列表、嵌套 QTabWidget 双层白框、控件内联随意 setStyleSheet 破坏全局主题

## 必须执行

### A. 全局主题加载（main.cpp）
1. `QApplication::setStyle(QStyleFactory::create("Fusion"))`
2. 设置 QPalette 浅色调色板（Window `#F4F6F9`、Base `#FFFFFF`、Highlight `#2563EB`）
3. 从 `:/styles/app.qss` 读取 QSS，`app.setStyleSheet(...)` 全局应用
4. Windows 上务必 Fusion + Palette，否则 QLineEdit/QComboBox 可能黑底黑字

### B. 使用 objectName，不要硬编码颜色
- 主按钮：`setObjectName("PrimaryButton")`
- 次按钮/返回/编辑：`setObjectName("GhostButton")`
- 卡片容器：`setObjectName("SectionCard")`
- 列表卡片：`setObjectName("FeedCard")`
- 子 Tab：`setObjectName("SubTabButton")` + checked 态；内容区 `#SubTabPane`
- 表格：`setObjectName("DataTable")`
- 标题/次要文字：`PlaceholderTitle` / `MutedLabel` / `SectionTitle`
- 顶栏：`TopBar`、`PageTitle`、`RoleBadge`
- 侧栏树：`NavTree`
- 内容栈：`ContentStack`（QStackedWidget）

### C. 布局间距（全站统一）
| 区域 | margins | spacing |
|---|---|---|
| 业务页根 layout | 28, 20, 28, 20 | 14 |
| SectionCard 内 | 14–20 | 8–12 |
| FeedCard 内 | 18, 16, 18, 16 | 8 |
| 顶栏 TopBar 高 | 56px | — |
| 侧栏项高 | 38px | — |

### D. 页面结构模板

**知识社区页**
```
QVBoxLayout(margin 0) → QStackedWidget#ContentStack
  └ listPage (margin 28,20)
       ├ 顶行：PlaceholderTitle「知识社区」 + stretch + QLineEdit(搜索)
       ├ 排序：PrimaryButton「最新」+ GhostButton「最热」（互斥，切换时 unpolish/polish）
       ├ SectionCard：常用标签 + GhostButton「编辑」+ 标签按钮行
       ├ QScrollArea(frameless) → FeedCard 列表
       └ MutedLabel 加载状态
```

**个人中心 / 用户主页**
```
QVBoxLayout(28,20)
  ├ GhostButton「← 返回」（仅他人主页）
  ├ SectionCard：AvatarLabel(56) + PlaceholderTitle + MutedLabel + GhostButton改密 + QCheckBox隐私
  └ SectionCard（margin 0 单层）
       ├ SubTabButton「发布的知识」「收藏的知识」
       └ QStackedWidget → QScrollArea → FeedCard / 空状态 MutedLabel居中
```
⚠️ 个人中心不要用 QTabWidget，用 SubTabButton + StackedWidget 避免双层白框。

**详情页**
```
margin 0 根布局
  ├ GhostButton 返回
  └ QScrollArea → SectionCard
       ├ 标题 PlaceholderTitle
       ├ 作者行：AvatarLabel(36) + 可点击用户名 GhostButton flat
       ├ 标签 chips（TagStyle）
       ├ QTextBrowser#ArticleContent
       └ 评论 SectionCard
```

### E. 组件样式要点
- **AvatarLabel**：圆 `#2563EB` 底、白字、首字、`border-radius: size/2`
- **FeedCard**：hover 边框 `#93C5FD`、背景 `#F8FAFC`、`setCursor(PointingHandCursor)`
- **标签 chip**：用 `TagStyle::createTagNameChip`（浅色底+同色字），常用标签筛选按钮用 Primary/Ghost 与检索页一致
- **QScrollArea**：`setFrameShape(QFrame::NoFrame)`，背景 transparent
- **对话框**：底栏 Cancel=GhostButton、OK=PrimaryButton；表单区 SectionCard

### F. 禁止事项
- ❌ 在业务代码里大面积 `widget->setStyleSheet("background:...")`（除 AvatarLabel、TagChip、富文本特例）
- ❌ QTableWidget 做社区信息流（改用 FeedCard）
- ❌ 页面背景纯白无层次（ContentStack 必须 `#F4F6F9`）
- ❌ 忘记 `QLabel { background: transparent }` 导致文字块白底割裂

## 交付
1. 提供完整 `resources/styles/app.qss`（可复制参考项目）
2. 修改 `main.cpp` 加载 QSS + Fusion + Palette
3. 扫描全项目 `setObjectName`，补齐上表命名
4. 输出迁移前后截图说明的关键页：登录、侧栏、知识社区、个人中心、详情

请先列出所有使用了非规范样式（硬编码颜色、缺 objectName）的文件，再逐页改。
```

---

## 2. 设计令牌（Design Tokens）

### 2.1 色彩

| 令牌 | 色值 | 用途 |
|---|---|---|
| `primary` | `#2563EB` | 主按钮、选中、链接 hover、头像底 |
| `primary-hover` | `#1D4FD7` | 主按钮 hover |
| `primary-pressed` | `#1A45BE` | 主按钮 pressed |
| `primary-light` | `#EEF2FF` | 角色徽章底、Tab hover、选中浅底 |
| `primary-border-hover` | `#93C5FD` | FeedCard hover 边框 |
| `page-bg` | `#F4F6F9` | 主窗口、ContentStack、登录页底 |
| `surface` | `#FFFFFF` | 卡片、输入框、顶栏、侧栏 |
| `border` | `#E5E8EC` | 卡片/表格/分隔线 |
| `border-input` | `#D8DCE2` | 输入框、Ghost 按钮边框 |
| `text-primary` | `#111827` | 标题、表头强调 |
| `text-body` | `#1F2A37` | 正文 |
| `text-secondary` | `#374151` | 侧栏、标签 |
| `text-muted` | `#6B7280` | 次要说明、统计数字 |
| `text-hint` | `#9098A3` | placeholder、hint |
| `error` | `#DC2626` | 错误文案 |
| `success` | `#059669` | 成功态（API 日志等） |
| `row-alt` | `#FAFBFC` | 表格斑马纹 |
| `row-hover` | `#F8FAFC` | 列表 hover |
| `select-bg` | `#E8F0FE` | 表格/列表选中 |

### 2.2 圆角与尺寸

| 令牌 | 值 |
|---|---|
| `radius-sm` | 6px（徽章、tag chip） |
| `radius-md` | 8px（按钮、输入框、列表项） |
| `radius-lg` | 12px（SectionCard、FeedCard、StatCard） |
| `radius-xl` | 14px（登录卡片、PlaceholderCard） |
| `topbar-height` | 56px |
| `nav-item-height` | 38px |
| `button-padding-primary` | 9px 18px |
| `button-padding-ghost` | 7px 14px |
| `font-base` | 14px |
| `font-title-page` | 17px（顶栏 PageTitle） |
| `font-title-section` | 16px（SectionTitle） |
| `font-title-card` | 16px semibold（FeedCardTitle） |
| `font-stat-value` | 28px bold（仪表盘） |

### 2.3 字体栈

```css
font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI", "PingFang SC", sans-serif;
```

---

## 3. objectName 完整词典

实现时 **必须** `setObjectName`，由 QSS 选择器驱动样式，业务代码不写颜色。

### 3.1 布局壳层

| objectName | 控件类型 | 说明 |
|---|---|---|
| `TopBar` | QFrame | 顶栏容器，高 56，底边框 |
| `PageTitle` | QLabel | 当前页标题 17px 600 |
| `RoleBadge` | QLabel | 角色 pill，蓝字浅蓝底 |
| `ContentStack` | QStackedWidget | 主内容区，背景 page-bg |
| `PageContent` | QWidget | 页面内容透明底 |
| `NavTree` | QTreeWidget | 左侧导航 |

### 3.2 按钮

| objectName | 场景 |
|---|---|
| `PrimaryButton` | 确认、搜索、选中排序「最新」、选中标签「全部」 |
| `GhostButton` | 取消、返回、编辑、退出、未选中排序/标签 |
| `SubTabButton` | 个人中心/开放API/日志 等子 Tab（checkable） |
| `LinkButton` | QToolButton 文字链接 |

### 3.3 卡片与列表

| objectName | 场景 |
|---|---|
| `SectionCard` | 白卡片分区：标签区、表单区、个人中心内容区 |
| `FeedCard` | 知识社区/个人中心 信息流条目 |
| `FeedCardTitle` | 卡片标题 |
| `FeedCardStats` | 👍💬👁 统计行 |
| `StatCard` | 仪表盘指标 |
| `PlaceholderCard` | 占位/无权限页 |
| `LoginCard` | 登录表单卡片 |
| `SubTabPane` | SubTab 下方内容 pane（可选包一层 QFrame） |
| `SubTabBar` | SubTab 按钮行容器 |
| `DataTable` | QTableWidget 业务表格（borderless 在 SectionCard 内） |

### 3.4 文字

| objectName | 场景 |
|---|---|
| `PlaceholderTitle` | 大标题（社区名、用户姓名） |
| `SectionTitle` | 区块小标题 |
| `MutedLabel` | 灰色说明、空状态、账号信息 |
| `HintLabel` | 表单 hint |
| `PageInfoLabel` | 分页/加载进度 |
| `LoadingLabel` | 加载中 |
| `ErrorLabel` | 错误 |
| `WelcomeTitle` / `WelcomeHint` | 仪表盘横幅 |

### 3.5 内容与互动

| objectName | 场景 |
|---|---|
| `ArticleContent` | QTextBrowser 详情正文 |
| `CommentItem` | 单条评论卡片 |
| `TagNameChip` | 标签名 chip（动态颜色用 TagStyle） |

---

## 4. 全局 QSS 加载规范

### 4.1 main.cpp 模板

```cpp
#include <QApplication>
#include <QColor>
#include <QFile>
#include <QPalette>
#include <QStyleFactory>

static void applyLightPalette(QApplication &app) {
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

// 在 main 中：
QApplication::setStyle(QStyleFactory::create("Fusion"));
applyLightPalette(app);
QFile qss(":/styles/app.qss");
if (qss.open(QFile::ReadOnly | QFile::Text)) {
    app.setStyleSheet(QString::fromUtf8(qss.readAll()));
}
```

### 4.2 resources.qrc

```xml
<RCC>
    <qresource prefix="/">
        <file alias="styles/app.qss">resources/styles/app.qss</file>
    </qresource>
</RCC>
```

### 4.3 迁移步骤

1. **整文件复制** 参考项目 `knowledgeAnswer/resources/styles/app.qss` → 目标项目同路径  
2. 替换 `main.cpp` 为 §4.1 模式  
3. 全局搜索 `setStyleSheet(`，仅保留：AvatarLabel、TagStyle、极少数富文本  
4. 全局搜索 `QPushButton(` / `new QLabel`，补 `setObjectName`  
5. 将所有 `QTabWidget` 社区/个人中心页改为 SubTabButton 模式（若出现双层白框）

---

## 5. 编码规范（样式相关）

### 5.1 按钮选中态切换（排序「最新/最热」）

切换 Primary/Ghost 后 **必须** refresh 样式：

```cpp
latestBtn->setObjectName(hot ? "GhostButton" : "PrimaryButton");
hotBtn->setObjectName(hot ? "PrimaryButton" : "GhostButton");
latestBtn->style()->unpolish(latestBtn);
latestBtn->style()->polish(latestBtn);
hotBtn->style()->unpolish(hotBtn);
hotBtn->style()->polish(hotBtn);
```

### 5.2 SubTab 辅助函数（全项目复用）

```cpp
QPushButton *makeSubTabButton(const QString &text, QWidget *parent) {
    auto *btn = new QPushButton(text, parent);
    btn->setObjectName("SubTabButton");
    btn->setCheckable(true);
    btn->setAutoExclusive(true);
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}
```

### 5.3 FeedCard 构造

```cpp
setObjectName("FeedCard");
setCursor(Qt::PointingHandCursor);
// 子 label：FeedCardTitle, MutedLabel(preview), FeedCardStats
```

### 5.4 标签颜色（TagStyle）

- DB 字段 `kb_tag.color` 存 `#RRGGBB`  
- 展示：`TagStyle::createTagNameChip(name, color, parent)`  
- 常用标签 **筛选按钮** 不要用 TagStyle，用 PrimaryButton/GhostButton 与「全部」一致

### 5.5 AvatarLabel

- 个人中心 56px，详情作者 36px  
- `setDisplayName(realName 优先)`  
- 样式在组件内 inline，不写入 app.qss

### 5.6 表格页（管理后台）

```
SectionCard
  └ filter 行：QLineEdit + PrimaryButton「查询」+ GhostButton 操作
  └ QTableWidget#DataTable（setShowGrid(false), alternatingRowColors）
  └ GhostButton 上一页/下一页 + PageInfoLabel
```

### 5.7 对话框

```
QVBoxLayout
  └ HintLabel（可选）
  └ SectionCard → 表单
  └ QHBoxLayout stretch + GhostButton cancel + PrimaryButton ok
```

---

## 6. 页面级视觉线框

### 6.1 知识社区

```
┌─────────────────────────────────────────────────────────────┐
│ TopBar: PageTitle                          RoleBadge User … │
├──────────┬──────────────────────────────────────────────────┤
│ NavTree  │  知识社区                    [ 模糊搜索........ ] │
│          │  [最新] [最热]                                    │
│          │  ┌─ SectionCard ─────────────────────────────┐   │
│          │  │ 常用标签                        [编辑]    │   │
│          │  │ [全部] [售前] [售后] [5G] ...           │   │
│          │  └─────────────────────────────────────────┘   │
│          │  ┌─ FeedCard ────────────────────────────────┐ │
│          │  │ 标题                                         │ │
│          │  │ 正文前二十字预览…                            │ │
│          │  │ 👍 10  💬 3  👁 100                          │ │
│          │  └─────────────────────────────────────────────┘ │
│          │  … scroll …                                       │
│          │  已加载 15 / 42 条，继续下拉…                     │
└──────────┴──────────────────────────────────────────────────┘
背景 #F4F6F9，卡片 #FFFFFF
```

### 6.2 个人中心

```
┌─ SectionCard ─────────────────────────────────────┐
│  (头像)  张三                                      │
│          账号：zhang · 已上线知识 5 篇              │
│          [修改密码]  ☐ 收藏仅自己可见               │
└───────────────────────────────────────────────────┘
┌─ SectionCard ─────────────────────────────────────┐
│ [发布的知识] [收藏的知识]   ← SubTabButton          │
│ ┌─ FeedCard ─────────────────────────────────────┐ │
│ │ …                                               │ │
│ └─────────────────────────────────────────────────┘ │
│           暂无收藏的知识  ← MutedLabel 居中         │
└─────────────────────────────────────────────────────┘
```

---

## 7. 与旧项目常见差异对照

| 旧项目常见问题 | 本规范做法 |
|---|---|
| Win11 深色模式黑底 | Fusion + QPalette + QSS |
| 原生 QTable 当 feed | ArticleFeedCard |
| 控件默认灰边厚重 | 8px 圆角 + `#E5E8EC` 细边框 |
| Tab 嵌套滚动区白块 | SubTabButton + StackedWidget |
| 标题字号不统一 | PlaceholderTitle / SectionTitle 分级 |
| 侧栏选中不明显 | NavTree selected `#E8F0FE` + 蓝字 |
| 按钮全是 QPushButton 默认 | Primary / Ghost 二分 |
| 标签纯色块太艳 | TagStyle 38% 透明度背景 |

---

## 8. 验收清单（样式专项）

- [ ] 深色系统主题下登录页、输入框、下拉框文字可读  
- [ ] ContentStack 背景灰、卡片白，层次清晰  
- [ ] 全站主按钮蓝色一致，无绿色/系统色混入  
- [ ] 知识社区 FeedCard hover 有反馈  
- [ ] 个人中心无多余空白 Tab 框  
- [ ] 详情正文 ArticleContent 白底圆角  
- [ ] 侧栏选中态蓝色高亮  
- [ ] 表格 DataTable 在 SectionCard 内无双重边框  
- [ ] 滚动条细圆角灰条，非 Windows 粗滚动条  
- [ ] 无业务页大面积 inline setStyleSheet（除 Avatar/Tag）

---

## 9. 文件清单（样式体系）

| 路径 | 作用 |
|---|---|
| `resources/styles/app.qss` | 全局 QSS（~620 行，可直接复制） |
| `resources.qrc` | 注册 qss 资源 |
| `main.cpp` | Fusion + Palette + 加载 QSS |
| `common/AvatarLabel.*` | 头像 |
| `common/TagStyle.*` | 标签 chip 动态色 |
| `common/widgets/AppTopBar.*` | TopBar 壳 |
| `common/widgets/NavSidebar.*` | NavTree 壳 |

---

## 10. 合并到功能落地提示词的一句话

在《知识社区与个人中心-跨项目落地指南》的 AI 提示词末尾追加：

> **UI 样式强制遵循** `docs/Qt前端样式规范与迁移提示词.md`：复制 app.qss、main.cpp Fusion 加载、全部使用 objectName 词典，个人中心/社区页按 §6 线框实现，验收 §8 清单。

---

*文档版本：2026-06-23 · QSS 源文件：`knowledgeAnswer/resources/styles/app.qss`*
