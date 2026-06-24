# 知识朗读（TTS）模块 — 实现说明与迁移文档

> **用途**：将 tuandui 项目中已验证的「知识详情朗读」能力，迁移到功能相同的兄弟项目。  
> **范围**：仅迁移语音播放模块本身；编码风格、命名空间、提示组件等**按目标项目既有规范**即可，本文不规定风格。  
> **技术**：Qt 6 · `QTextToSpeech` · Windows SAPI · 纯前端（无后端接口）  
> **参考实现**：`knowledgeAnswer/modules/knowledge/search/ArticleDetailPanel.{h,cpp}`（2026-06-24）

---

## 1. 模块做什么

在**知识详情页**增加一个按钮，用户点击后由本机语音引擎朗读：

```text
标题 → 摘要 → 正文（HTML 会先转成纯文本）
```

| 能力 | 说明 |
|---|---|
| 朗读 | 单按钮，未播放时显示「朗读」 |
| 停止 | 播放中按钮变为「停止」，点击结束（**不是暂停**） |
| 自动停止 | 切换文章、返回列表、页面销毁时停止 |
| 文本清洗 | 去掉 HTML 标签，不读 `<table>`、`<a>` 等标记 |
| 离线 | 不调 REST API，密钥/配置均不需要 |

---

## 2. 实现原理

### 2.1 为什么用 Qt TextToSpeech

Qt 自带 [`QTextToSpeech`](https://doc.qt.io/qt-6/qtexttospeech.html)，在 Windows 上通过 **SAPI 插件**（`qtexttospeech_sapi.dll`）调用系统 TTS，无需接入 Azure/百度等云服务，也无需改 Java/Python 后端。

```text
详情页按钮
    → m_tts->say(纯文本)
        → Qt TextToSpeech
            → qtexttospeech_sapi.dll
                → Windows 语音（如 Microsoft Huihui Desktop）
                    → 扬声器
```

### 2.2 为什么不读 QTextBrowser 里的内容

详情正文通常用 `QTextBrowser::setHtml()` 展示，HTML 里含有摘要区块、样式、`<strong>摘要：</strong>` 等**展示用** markup。  
朗读应基于 API 返回的**原始字段**拼接：

| 字段 | 用途 |
|---|---|
| `title` | 标题，原样朗读 |
| `summary` | 摘要，原样朗读（可选，无则跳过） |
| `content` | 正文，**可能是 HTML**，须 `htmlToPlainText()` 后再读 |

**错误做法**：`m_contentBrowser->toPlainText()` — 会把页面上所有可见 HTML 转成文本，含「摘要：」标签等冗余内容。

### 2.3 状态与按钮

`QTextToSpeech::State`（Qt 6.7+）主要用到：

| 状态 | 含义 | 按钮 |
|---|---|---|
| `Ready` | 空闲 | 「朗读」 |
| `Speaking` | 正在播 | 「停止」 |
| `Synthesizing` | 合成中（长文本） | 「停止」 |
| `Error` | 引擎异常 | 「朗读」禁用 |

交互逻辑：**一个按钮两态切换**，不单独做「暂停 / 继续」（见 §4）。

---

## 3. 模块结构

朗读逻辑可直接写在详情页类里（tuandui 做法），也可抽成独立类。无论哪种，**内部都必须包含下列职责**：

```text
┌─────────────────────────────────────────────────┐
│  详情页（或 ArticleSpeechReader）                  │
│                                                 │
│  m_speechText      缓存待朗读纯文本               │
│  m_tts             QTextToSpeech 实例             │
│  m_readAloudBtn    朗读/停止 按钮                 │
│                                                 │
│  htmlToPlainText()      HTML → 纯文本            │
│  buildSpeechText()      拼 title/summary/content │
│  createTextToSpeech()   创建并配置引擎            │
│  abandonSpeechEngine()  安全停止（核心）          │
│  stopSpeech()           对外停止入口              │
│  updateSpeechButtons()  刷新按钮                  │
│  onReadAloud()          点击处理                   │
└─────────────────────────────────────────────────┘
```

### 3.1 需要在详情页挂接的 4 个时机

| 时机 | 调用 |
|---|---|
| 构造函数末尾 | `setupTextToSpeech()` |
| 详情数据渲染完成 | `buildSpeechText(detail)` + `updateSpeechButtons()` |
| 加载新文章 / 打开详情前 | `stopSpeech()` + `m_speechText.clear()` |
| 返回列表 / 析构 | `stopSpeech()` 或 `abandonSpeechEngine(m_tts)` |

---

## 4. 关键约束：禁止调用 stop() / pause()

这是本模块最重要的实现细节，迁移时必须遵守。

### 4.1 现象

在 **Qt 6.7 + Windows + SAPI** 环境下，播放中调用：

```cpp
m_tts->stop();
m_tts->pause();
m_tts->resume();
```

会导致 **SIGSEGV（段错误）** 或 UI 卡死。tuandui 已在 `knowledgeAnswer/tests/test_tts_stop.cpp` 中复现。

### 4.2 正确做法：丢弃引擎（abandon）

不调用 `stop()`，而是：

1. **静音**：`setVolume(0.0)`，用户听不到后续内容  
2. **脱钩**：`disconnect(this)` + `setParent(nullptr)`，与详情页脱离  
3. **延迟销毁**：  
   - 若已 `Ready` → 直接 `deleteLater()`  
   - 若仍在 `Speaking` → 等 `stateChanged(Ready)` 后再 `deleteLater()`  
4. **立即新建**一个 `QTextToSpeech` 给详情页继续用  

```text
用户点「停止」
    │
    ├─ oldEngine.setVolume(0)     // 静音，不用 stop()
    ├─ oldEngine.setParent(nullptr) // orphan，自行播完后销毁
    │
    └─ m_tts = createTextToSpeech() // 新引擎，状态 Ready
```

**副作用**：每次停止会短暂存在一个 orphan 引擎（静音播完即销毁），对桌面应用可接受。

---

## 5. 完整实现（按步骤）

以下代码为 tuandui 验证通过的参考实现。类名 `ArticleDetailPanel`、DTO `ArticleDetail`、提示 `notify::warn` 等**替换为目标项目对应写法**即可，逻辑保持不变。

### 5.1 CMake 依赖

```cmake
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets TextToSpeech)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets TextToSpeech)

target_link_libraries(${YOUR_TARGET} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::TextToSpeech
)
```

`.pro` 工程：`QT += texttospeech`

安装 Qt 时需勾选 **TextToSpeech** 组件。Release 打包执行 `windeployqt`，确认存在：

- `Qt6TextToSpeech.dll`
- `plugins/texttospeech/qtexttospeech_sapi.dll`

### 5.2 头文件新增（详情页 .h）

```cpp
class QTextToSpeech;
class QPushButton;

// private:
void setupTextToSpeech();
QTextToSpeech *createTextToSpeech();
void abandonSpeechEngine(QTextToSpeech *engine);
void stopSpeech();
void buildSpeechText(const ArticleDetail &detail);  // 换成你的 DTO
void updateSpeechButtons();
void onReadAloud();
void onTtsStateChanged();

QString m_speechText;
QTextToSpeech *m_tts = nullptr;
QPushButton *m_readAloudBtn = nullptr;
```

若原先无析构函数，增加 `~ArticleDetailPanel() override;`。

### 5.3 cpp 依赖

```cpp
#include <QLocale>
#include <QTextDocument>
#include <QTextToSpeech>
#include <QVoice>
```

### 5.4 HTML 转纯文本

放在 cpp 文件匿名命名空间内，供 `buildSpeechText` 使用：

```cpp
namespace {

QString htmlToPlainText(const QString &html) {
    if (html.isEmpty()) {
        return {};
    }
    QTextDocument doc;
    doc.setHtml(html);
    return doc.toPlainText().trimmed();
}

} // namespace
```

`QTextDocument::setHtml` + `toPlainText` 会去掉标签、把表格/列表转为可读文本，满足「文本清洗」需求。

### 5.5 拼接朗读稿

```cpp
void ArticleDetailPanel::buildSpeechText(const ArticleDetail &detail) {
    QString text = detail.title.trimmed();

    if (!detail.summary.isEmpty()) {
        if (!text.isEmpty()) {
            text += QLatin1Char('\n');
        }
        text += detail.summary.trimmed();
    }

    const QString body = htmlToPlainText(detail.content);
    if (!body.isEmpty()) {
        if (!text.isEmpty()) {
            text += QLatin1String("\n\n");
        }
        text += body;
    }

    m_speechText = text;
}
```

若目标 DTO 无 `summary`，删除中间那段即可；`content` 字段名不同则在形参里映射。

### 5.6 创建语音引擎

**必须显式指定 `"sapi"` 引擎**（Windows 上比默认 `winrt` 更稳定）：

```cpp
QTextToSpeech *ArticleDetailPanel::createTextToSpeech() {
    auto *tts = new QTextToSpeech(QStringLiteral("sapi"), this);
    tts->setLocale(QLocale(QLocale::Chinese, QLocale::China));

    // 优先选中文发音人
    for (const QVoice &voice : tts->availableVoices()) {
        if (voice.locale().language() == QLocale::Chinese) {
            tts->setVoice(voice);
            break;
        }
    }

    tts->setRate(0.0);    // -1.0 ~ 1.0，0 为正常语速
    tts->setVolume(1.0);  // 0.0 ~ 1.0

    connect(tts, &QTextToSpeech::stateChanged, this, &ArticleDetailPanel::onTtsStateChanged);
    connect(tts, &QTextToSpeech::errorOccurred, this,
            [this](QTextToSpeech::ErrorReason reason, const QString &errorString) {
                Q_UNUSED(reason);
                // 换成目标项目的提示方式
                notify::warn(this, errorString.isEmpty()
                                  ? QStringLiteral("朗读失败，请检查系统是否已安装中文语音包")
                                  : errorString);
                updateSpeechButtons();
            });
    return tts;
}

void ArticleDetailPanel::setupTextToSpeech() {
    m_tts = createTextToSpeech();
    const bool backendOk = m_tts->state() != QTextToSpeech::Error
                           && m_tts->errorReason() == QTextToSpeech::ErrorReason::NoError;
    m_readAloudBtn->setEnabled(backendOk);
    if (!backendOk) {
        m_readAloudBtn->setToolTip(QStringLiteral("未检测到可用的语音引擎"));
    }
}
```

**参数说明**：

| 调用 | 作用 |
|---|---|
| `QTextToSpeech("sapi", parent)` | 绑定 Windows SAPI 插件 |
| `setLocale(Chinese, China)` |  locale 为中文 |
| `setVoice(...)` | 选 Huihui / Xiaoyi 等中文 voice |
| `setRate(0.0)` | 正常语速，可按需调整 |
| `stateChanged` | 驱动按钮「朗读 ↔ 停止」切换 |
| `errorOccurred` | 引擎初始化失败、无 voice 等 |

### 5.7 安全停止（核心）

```cpp
void ArticleDetailPanel::abandonSpeechEngine(QTextToSpeech *engine) {
    if (!engine) {
        return;
    }

    const auto state = engine->state();
    const bool active = state == QTextToSpeech::Speaking
                        || state == QTextToSpeech::Synthesizing;
    if (active) {
        engine->setVolume(0.0);
    }

    engine->disconnect(this);
    engine->setParent(nullptr);

    if (state == QTextToSpeech::Ready) {
        engine->deleteLater();
        return;
    }

    connect(engine, &QTextToSpeech::stateChanged, engine, [engine](QTextToSpeech::State s) {
        if (s != QTextToSpeech::Ready) {
            return;
        }
        engine->disconnect();
        engine->deleteLater();
    });
}

void ArticleDetailPanel::stopSpeech() {
    if (!m_tts) {
        updateSpeechButtons();
        return;
    }

    const bool active = m_tts->state() == QTextToSpeech::Speaking
                        || m_tts->state() == QTextToSpeech::Synthesizing;
    if (!active) {
        updateSpeechButtons();
        return;
    }

    auto *old = m_tts;
    m_tts = nullptr;
    abandonSpeechEngine(old);
    m_tts = createTextToSpeech();
    updateSpeechButtons();
}
```

**析构函数**同样走 `abandonSpeechEngine`，**不要**在析构里调 `stop()`：

```cpp
ArticleDetailPanel::~ArticleDetailPanel() {
    if (m_tts) {
        abandonSpeechEngine(m_tts);
        m_tts = nullptr;
    }
}
```

### 5.8 按钮 UI 与交互

**布局**：详情页顶栏，「返回」左侧，「朗读」右侧（`addStretch()` 隔开）。

```cpp
m_readAloudBtn = new QPushButton(QStringLiteral("朗读"), this);
// objectName / 样式按目标项目 QSS 设置
connect(m_readAloudBtn, &QPushButton::clicked, this, &ArticleDetailPanel::onReadAloud);
```

**刷新按钮状态**：

```cpp
void ArticleDetailPanel::updateSpeechButtons() {
    if (!m_tts || !m_readAloudBtn) {
        return;
    }

    const bool backendOk = m_tts->state() != QTextToSpeech::Error
                           && m_tts->errorReason() == QTextToSpeech::ErrorReason::NoError;
    const bool hasText = !m_speechText.isEmpty();
    const bool speaking = m_tts->state() == QTextToSpeech::Speaking
                          || m_tts->state() == QTextToSpeech::Synthesizing;

    m_readAloudBtn->setEnabled(backendOk && (hasText || speaking));
    m_readAloudBtn->setText(speaking ? QStringLiteral("停止") : QStringLiteral("朗读"));
}

void ArticleDetailPanel::onReadAloud() {
    if (!m_tts) {
        return;
    }

    const bool speaking = m_tts->state() == QTextToSpeech::Speaking
                          || m_tts->state() == QTextToSpeech::Synthesizing;
    if (speaking) {
        stopSpeech();
        return;
    }

    if (m_speechText.isEmpty()) {
        notify::warn(this, QStringLiteral("暂无正文可朗读"));
        return;
    }
    m_tts->say(m_speechText);
}

void ArticleDetailPanel::onTtsStateChanged() {
    updateSpeechButtons();
}
```

**点击流程**：

```text
[朗读] ──say()──▶ Speaking ──按钮变「停止」
                      │
          用户点「停止」──stopSpeech()──▶ abandon + 新引擎 ──▶ Ready ──按钮变「朗读」
```

### 5.9 构造与生命周期挂接

```cpp
ArticleDetailPanel::ArticleDetailPanel(...) {
    buildUi();           // 其中创建 m_readAloudBtn
    setupTextToSpeech();
}

void ArticleDetailPanel::openArticle(qint64 articleId) {
    stopSpeech();
    m_speechText.clear();
    updateSpeechButtons();
    // ... 请求 API ...
}

void ArticleDetailPanel::renderDetail(const ArticleDetail &detail) {
    // ... 渲染标题、HTML 正文到 QTextBrowser ...
    buildSpeechText(detail);
    updateSpeechButtons();
}

// 返回按钮：先 stopSpeech()，再 emit backRequested()
connect(m_backBtn, &QPushButton::clicked, this, [this]() {
    stopSpeech();
    emit backRequested();
});
```

---

## 6. 端到端数据流（汇总）

```text
API 返回 ArticleDetail
        │
        ▼
renderDetail()
   ├─ setHtml(content)          → 屏幕展示
   └─ buildSpeechText()         → m_speechText 缓存
        │
        ▼
用户点「朗读」
        │
        ▼
m_tts->say(m_speechText)        → SAPI 播放
        │
        ├─ stateChanged(Speaking) → 按钮「停止」
        │
        ├─ 用户点「停止」
        │      └─ stopSpeech() → abandon + createTextToSpeech()
        │
        ├─ 切文章 / 返回
        │      └─ stopSpeech()
        │
        └─ stateChanged(Ready)   → 按钮「朗读」（自然播完）
```

---

## 7. 迁移步骤（ checklist ）

1. [ ] `CMakeLists.txt` / `.pro` 增加 `TextToSpeech`
2. [ ] 详情页 `.h` 增加 §5.2 成员与方法声明
3. [ ] 详情页 `.cpp` 增加 §5.3 ~ §5.9 全部实现
4. [ ] 顶栏加「朗读」按钮并 connect
5. [ ] `renderDetail` 末尾调用 `buildSpeechText` + `updateSpeechButtons`
6. [ ] `openArticle` / 返回 / 析构 挂接 `stopSpeech` 或 `abandonSpeechEngine`
7. [ ] 全文搜索确认**无** `->stop(` / `->pause(` / `->resume(`
8. [ ] 编译、`windeployqt`、§8 手工测试

**无需改动**：后端、数据库、API 契约、网络层。

---

## 8. 测试

### 8.1 手工用例

| # | 操作 | 预期 |
|---|---|---|
| 1 | 打开有正文的知识 | 「朗读」可点 |
| 2 | 点「朗读」 | 听到标题+摘要+正文；按钮变「停止」 |
| 3 | 播放中点「停止」 | 声音停；按钮变「朗读」；**不崩溃** |
| 4 | 停止后再点「朗读」 | 从头播放 |
| 5 | 播放中切换文章 | 前一篇停止 |
| 6 | 播放中返回列表 | 停止 |
| 7 | 空正文 | 按钮禁用或提示「暂无正文可朗读」 |
| 8 | HTML 正文含表格/链接 | 不读标签名 |

### 8.2 可选冒烟脚本

复制 `knowledgeAnswer/tests/test_tts_smoke.cpp` 到目标项目，验证 `engine=sapi`、中文 voice 可用。

---

## 9. 常见问题

| 现象 | 原因 | 处理 |
|---|---|---|
| 链接失败 `QTextToSpeech` | 未链模块 | 检查 CMake / `QT += texttospeech` |
| 运行缺 DLL | 未 deploy | `windeployqt your.exe` |
| 按钮灰色 | 无语音引擎 / 未装中文语音包 | Qt 安装 TextToSpeech；Windows 设置 → 语音 → 中文 |
| 读英文 | 未选中中文 voice | 检查 `createTextToSpeech` 中 voice 循环 |
| 点停止崩溃 | 调用了 `stop()` | 必须用 §5.7 abandon 方案 |
| 停止后极短余音 | SAPI 无法硬中断 | 已静音；可接受 |
| Linux / macOS | 无 sapi 引擎 | 构造时不传 `"sapi"`，用默认引擎；**仍需实测停止是否安全** |

---

## 10. 参考源文件

| 文件 | 内容 |
|---|---|
| `ArticleDetailPanel.h` | TTS 成员与方法声明 |
| `ArticleDetailPanel.cpp` L39-46 | `htmlToPlainText` |
| `ArticleDetailPanel.cpp` L221-361 | TTS 全部逻辑 |
| `ArticleDetailPanel.cpp` L444-445 | `renderDetail` 内 `buildSpeechText` |
| `CMakeLists.txt` L12-13, L127 | TextToSpeech 依赖 |
| `tests/test_tts_smoke.cpp` | 引擎冒烟 |
| `tests/test_tts_stop.cpp` | stop 崩溃复现（勿在生产代码调用 stop） |

---

*与 tuandui `ArticleDetailPanel` TTS 实现同步 · 2026-06-24*
