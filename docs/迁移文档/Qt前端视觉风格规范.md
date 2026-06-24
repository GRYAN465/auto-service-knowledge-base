# Qt 客户端视觉风格规范

> 工程：`knowledgeAnswer/`  
> 主题文件：`resources/styles/app.qss`  
> 加载入口：`main.cpp`（Fusion 风格 + 浅色调色板 + 全局 QSS）  
> 设计参考：花笺笔记类暖色极简界面  
> 最近更新：2026-06-24（v1.3 导航/FeedCard 半透明柔化、欢迎横幅黄绿渐变）

---

## 1. 设计原则

| 原则 | 说明 |
|---|---|
| 暖色极简 | 乳白大底、低饱和米黄/淡绿点缀，避免高饱和蓝紫商务风 |
| 留白优先 | 卡片间距 ≥ 14px，内边距 ≥ 18px，不堆叠重色块 |
| 层次靠边框 | 侧栏/顶栏用 `#E0D9CE` 分隔线，卡片用 `#F1EEE8` 描边 |
| 正文高对比 | 正文 `#000000`，次要信息 `#4A4A4A` / `#757575` |
| 分区可辨 | **导航米黄 `#F3EDE4` 与主背景 `#FEFEFE` 有明显温差** |
| 交互柔化 | 导航 hover/选中用**半透明叠加**，避免黄绿强对比；FeedCard hover 轻暖、不用饱和绿边 |
| 样式集中 | **优先改 `app.qss`**，控件用 `objectName` 挂接，避免页面内硬编码颜色 |

---

## 2. 色板（Design Tokens）

### 2.1 核心色

| Token | 色值 | 用途 |
|---|---|---|
| `bg-main` | **#FEFEFE** | 主背景：内容区、卡片、对话框、正文区 |
| `bg-nav` | **#F3EDE4** | 导航栏、表头、问答助手气泡、Tab 未选中 |
| `bg-table-selected` | **#F0EBE4** | 表格行选中、列表选中、下拉列表选中 |
| `bg-accent-soft` | **#F1EEE8** | 次要按钮底、搜索栏/输入框底、卡片边框 |
| `bg-comment-hover` | **#FAF8F4** | 详情页评论 hover / 点选反馈 |
| `bg-selected` | **#EEF2EE** | 文本选区、输入框 selection（轻量高亮） |
| `text-accent` | **#9AA69D** | 主按钮底、指标提示、CardTitle |

**导航半透明叠加（NavTreeDelegate 硬编码，v1.3）**

| 状态 | 填充 | 文字 |
|---|---|---|
| hover | `rgba(254,254,254,72)` 白雾叠加 | `#000000` |
| 选中 | `rgba(154,166,157,48)` 淡绿叠加 | `#000000` + semibold |

> **对比度策略（v1.3）**  
> - 导航不再使用实色 `#DFEBE3` / `#EBE4D8` 与 `#5A6F63` 字，改为**半透明叠加 + 黑字**，黄绿对比更自然。  
> - FeedCard hover 改为 **`#FAF8F4` + 边 `#E8E2D8`**，取消饱和绿边框。  
> - 欢迎横幅 **`#F6F0E4` → `#EAF1EC` → `#F3EDE4`** 黄绿渐变。

### 2.2 衍生色

| Token | 色值 | 用途 |
|---|---|---|
| `text-primary` | **#000000** | 正文、标题、导航选中字 |
| `text-secondary` | **#4A4A4A** | 元信息、欢迎副标题 |
| `text-muted` | **#757575** | 占位提示、Loading、Tab 未选中 |
| `text-accent-dark` | **#6B7F74** | 链接、Tab 选中、RoleBadge |
| `text-header` | **#3D3D3D** | 表头文字 |
| `border-default` | **#E0D9CE** | 输入框/按钮描边、顶栏/侧栏分隔线 |
| `border-light` | **#F1EEE8** | 卡片描边、表格网格 |
| `border-feed-hover` | **#E8E2D8** | FeedCard hover 描边 |
| `border-selected` | **#9AA69D** | 输入框 focus |
| `primary-hover` | **#8A9690** | 主按钮 hover |
| `primary-pressed` | **#7A8A82** | 主按钮 pressed |
| `primary-disabled` | **#D4DAD6** | 主按钮禁用 |
| `error` | **#B94A48** | 错误提示 |
| `row-alt` | **#FAF8F4** | 表格斑马纹 |
| `bg-disabled` | **#F5F0E8** | 禁用输入框/下拉底 |

### 2.3 色板层级示意

```text
#FEFEFE  主画布（乳白）
   │
   ├── #FAF8F4  表格斑马纹 / FeedCard hover
   │
   ├── #F3EDE4  导航 / 表头 / 次要区块（米黄）
   │
   ├── #F1EEE8  控件 / 边框（淡黄）
   │
   ├── #F0EBE4  表格·列表选中
   │
   └── #9AA69D  主按钮 / 全局强调（鼠尾草绿）
        └── 导航 hover/选中：半透明叠加（见 §2.1）
```

### 2.4 对比度速查

| 相邻区域 | 前景 | 背景 | 说明 |
|---|---|---|---|
| 主内容 vs 侧栏 | — | #FEFEFE vs **#F3EDE4** | 侧栏偏暖黄 |
| 表格未选中 vs 选中 | #000000 | #FEFEFE vs **#F0EBE4** | 轻暖高亮 |
| 导航 hover / 选中 | **#000000** | 半透明叠加于 **#F3EDE4** | 无黄绿强跳色 |
| FeedCard 常态 vs hover | — | #FEFEFE vs **#FAF8F4** | 极轻反馈 |
| 欢迎横幅 | — | **#F6F0E4 → #EAF1EC → #F3EDE4** | 黄绿渐变 |

---

## 3. 字体与排版

| 项目 | 规范 |
|---|---|
| 字体栈 | `"Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI", "PingFang SC", sans-serif` |
| 基础字号 | 14px（全局 `QWidget`） |
| 页面标题 `#PageTitle` | 17px / semibold |
| 区块标题 `#SectionTitle` | 16px / semibold |
| 卡片标题 `#FeedCardTitle` | 16px / semibold |
| 欢迎标题 `#WelcomeTitle` | 22px / bold |
| 表格选中行 | semibold（QSS `font-weight: 600`） |
| 导航选中项 | semibold + `#000000` |

---

## 4. 圆角与间距

| 元素 | 圆角 | 内边距参考 |
|---|---|---|
| 按钮 | 8px | 主按钮 9×18；次要 7×14 |
| 卡片 `#SectionCard` / `#FeedCard` | 12px | 18–20px |
| 欢迎横幅 `#WelcomeBanner` | 12px | — |
| 导航项 | 8px | 高 38px |

---

## 5. 组件规范（objectName → QSS）

### 5.1 布局壳层

| objectName | 控件 | 背景 | 边框 |
|---|---|---|---|
| `TopBar` | 顶栏 QFrame | #FEFEFE | 底边 **#E0D9CE** |
| `NavSidebar` | 侧栏容器 | **#F3EDE4** | 右边 **#E0D9CE** |
| `NavTree` | 导航 QTreeWidget | **#F3EDE4** | 无 |
| `ContentStack` | 主内容 QStackedWidget | #FEFEFE | 无 |

**导航交互（NavTreeDelegate）**

```text
默认    透明底 + #000000 文字
hover   rgba(254,254,254,72) 半透明白雾
选中    rgba(154,166,157,48) 半透明淡绿 + #000000 semibold
```

### 5.2 按钮

| objectName | 场景 | 常态 | 激活/特殊 |
|---|---|---|---|
| `PrimaryButton` | 主操作 | 底 #9AA69D + 白字 | hover #8A9690 |
| `GhostButton` | 次要操作 | 底 #F1EEE8 + 黑字 | `[active="true"]` → `rgba(154,166,157,0.18)` 底 + #6B7F74 字 |

### 5.3 卡片与区块

| objectName | 背景 | 备注 |
|---|---|---|
| `FeedCard` | #FEFEFE | hover → 底 **#FAF8F4**，边 **#E8E2D8** |
| `WelcomeBanner` | 渐变 **#F6F0E4 → #EAF1EC → #F3EDE4** | 首页欢迎条 |
| `SectionCard` | #FEFEFE | 白底卡片 |
| `CommentItem` / `#CommentReply` | 透明底；hover → **#FAF8F4**（浅黄） | 详情页评论，无默认色块 |

### 5.4 表格与列表

| 部位 | 样式 |
|---|---|
| **选中行** | 底 **#F0EBE4**，字 #000000，semibold |
| 表头 | 底 **#F3EDE4**，字 **#3D3D3D** |

### 5.5 文字标签

| objectName | 颜色 | 用途 |
|---|---|---|
| `RoleBadge` | 底 `rgba(154,166,157,0.18)` / 字 **#6B7F74** | 顶栏角色 |
| `ErrorLabel` | #B94A48 | 错误 |

---

## 6. 工程接入

```cpp
// main.cpp — Fusion + QPalette + app.qss
// 导航半透明色见 common/NavTreeDelegate.cpp
```

---

## 7. 硬编码颜色清单

| 文件 | 内容 |
|---|---|
| `common/NavTreeDelegate.cpp` | hover `rgba(254,254,254,72)`；选中 `rgba(154,166,157,48)`；字 `#000000` |
| `main.cpp` | `QPalette::Highlight` → **#F0EBE4** |
| `ArticleDetailPanel.cpp` | HTML 模板 `pre` 底 → **#F3EDE4** |
| `common/AvatarLabel.cpp` | 头像底 `#9AA69D` |

---

## 8. 布局色块示意

```text
┌─────────────────────────────────────────────────────────────┐
│ TopBar (#FEFEFE)                                             │
├──────────────┬──────────────────────────────────────────────┤
│ NavSidebar   │ WelcomeBanner: 黄→绿渐变                      │
│ (#F3EDE4)    │ FeedCard hover: #FAF8F4（极轻）               │
│ [选中]半透明 │ DataTable 选中: #F0EBE4                       │
│ 淡绿叠加     │                                               │
└──────────────┴──────────────────────────────────────────────┘
```

---

## 9. 版本变更

| 版本 | 变更 |
|---|---|
| v1.0 | 花笺暖色主题 |
| v1.1 | 导航 `#F3EDE4`、表格选中 `#E5DDD0`、分隔线 `#E0D9CE` |
| v1.2 | 表格 `#F0EBE4`；导航实色 `#DFEBE3` + `#5A6F63` |
| **v1.3** | 导航改**半透明叠加**+黑字；FeedCard hover 柔化；WelcomeBanner **黄绿三色渐变**；评论区**透明底 + hover #FAF8F4** |

---

## 10. 验证清单

| # | 检查项 |
|---|---|
| 1 | 导航 hover/选中**无黄绿强对比**，过渡自然 |
| 2 | 知识社区 FeedCard hover **轻暖、不突兀** |
| 3 | 首页欢迎横幅为**黄→绿渐变** |
| 4 | 表格选中 `#F0EBE4` 仍清晰可辨 |
| 5 | 侧栏 `#F3EDE4` 与主区 `#FEFEFE` 可区分 |

---

## 11. 相关文件

| 文件 | 说明 |
|---|---|
| `resources/styles/app.qss` | 主题 QSS |
| `common/NavTreeDelegate.cpp` | 导航半透明绘制 |
| `main.cpp` | Fusion + QPalette |

---

*维护：改色先改 §2 Token → 同步 `app.qss` + NavTreeDelegate → 跑 §10 验证*
