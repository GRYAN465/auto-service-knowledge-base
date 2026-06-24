#include "app/AiConfigPage.h"

#include "core/auth/Session.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace kb {

AiConfigPage::AiConfigPage(const QString &title, QWidget *parent) : QWidget(parent), m_title(title) {
    if (!Session::instance().hasPermission(QStringLiteral("ai:config"))) {
        auto *root = new QVBoxLayout(this);
        root->addStretch();
        auto *deny = new QLabel(QStringLiteral("无权限访问 AI 配置"), this);
        deny->setAlignment(Qt::AlignCenter);
        deny->setObjectName("PlaceholderHint");
        root->addWidget(deny);
        root->addStretch();
        return;
    }
    buildUi();
    loadConfig();
}

void AiConfigPage::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 24);
    root->setSpacing(14);

    // ===== LLM 接入配置 =====
    auto *llmTitle = new QLabel(QStringLiteral("大模型接入（OpenAI 兼容）"), this);
    llmTitle->setObjectName("SectionTitle");
    root->addWidget(llmTitle);

    auto *card = new QFrame(this);
    card->setObjectName("FormCard");
    auto *form = new QFormLayout(card);
    form->setLabelAlignment(Qt::AlignRight);
    form->setContentsMargins(18, 16, 18, 16);
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(12);

    m_baseUrl = new QLineEdit(card);
    m_baseUrl->setPlaceholderText(QStringLiteral("如 https://api.example.com/v1"));
    form->addRow(QStringLiteral("接口地址"), m_baseUrl);

    m_apiKey = new QLineEdit(card);
    m_apiKey->setEchoMode(QLineEdit::Password);
    m_apiKey->setPlaceholderText(QStringLiteral("留空＝不修改"));
    form->addRow(QStringLiteral("API Key"), m_apiKey);

    m_model = new QLineEdit(card);
    m_model->setPlaceholderText(QStringLiteral("如 qwen-plus / gpt-4o-mini"));
    form->addRow(QStringLiteral("模型名"), m_model);

    m_temperature = new QDoubleSpinBox(card);
    m_temperature->setRange(0.0, 2.0);
    m_temperature->setSingleStep(0.1);
    m_temperature->setDecimals(2);
    m_temperature->setValue(0.2);
    m_temperature->setMaximumWidth(140);
    form->addRow(QStringLiteral("温度"), m_temperature);

    m_maxTokens = new QSpinBox(card);
    m_maxTokens->setRange(1, 32768);
    m_maxTokens->setSingleStep(128);
    m_maxTokens->setValue(1024);
    m_maxTokens->setMaximumWidth(140);
    form->addRow(QStringLiteral("最大生成长度"), m_maxTokens);

    m_enabled = new QCheckBox(QStringLiteral("启用大模型（关闭则问答始终走抽取式兜底）"), card);
    form->addRow(QStringLiteral("启用"), m_enabled);

    root->addWidget(card);

    auto *btnBar = new QHBoxLayout();
    m_testBtn = new QPushButton(QStringLiteral("测试连接"), this);
    m_testBtn->setObjectName("GhostButton");
    m_saveBtn = new QPushButton(QStringLiteral("保存并下发"), this);
    m_saveBtn->setObjectName("PrimaryButton");
    btnBar->addWidget(m_testBtn);
    btnBar->addWidget(m_saveBtn);
    btnBar->addStretch();
    root->addLayout(btnBar);

    m_status = new QLabel(this);
    m_status->setObjectName("StatusLabel");
    m_status->setWordWrap(true);
    root->addWidget(m_status);

    connect(m_testBtn, &QPushButton::clicked, this, &AiConfigPage::testConnection);
    connect(m_saveBtn, &QPushButton::clicked, this, &AiConfigPage::saveConfig);

    // 分隔
    auto *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setObjectName("HairLine");
    root->addWidget(line);

    // ===== 向量索引 =====
    auto *idxTitle = new QLabel(QStringLiteral("向量索引"), this);
    idxTitle->setObjectName("SectionTitle");
    root->addWidget(idxTitle);

    auto *idxHint = new QLabel(
        QStringLiteral("把全部在线知识重新向量化入库。平时上下线已自动同步，本操作用于初次回填或异常修复。"),
        this);
    idxHint->setObjectName("MutedLine");
    idxHint->setWordWrap(true);
    root->addWidget(idxHint);

    auto *idxBar = new QHBoxLayout();
    m_rebuildBtn = new QPushButton(QStringLiteral("重建全部在线知识索引"), this);
    m_rebuildBtn->setObjectName("GhostButton");
    const bool canIndex = Session::instance().hasPermission(QStringLiteral("ai:index"));
    m_rebuildBtn->setEnabled(canIndex);
    if (!canIndex) {
        m_rebuildBtn->setToolTip(QStringLiteral("需要 ai:index 权限"));
    }
    idxBar->addWidget(m_rebuildBtn);
    idxBar->addStretch();
    root->addLayout(idxBar);

    m_rebuildStatus = new QLabel(this);
    m_rebuildStatus->setObjectName("MutedLine");
    m_rebuildStatus->setWordWrap(true);
    root->addWidget(m_rebuildStatus);

    connect(m_rebuildBtn, &QPushButton::clicked, this, &AiConfigPage::rebuildIndex);

    root->addStretch();
}

void AiConfigPage::loadConfig() {
    QPointer<AiConfigPage> self(this);
    ApiClient::instance().get("/ai/llm-config", [self](const ApiResponse &r) {
        if (!self || !r.ok) {
            if (self && !r.ok) {
                self->setStatus(self->m_status, r.message, 2);
            }
            return;
        }
        const QJsonObject d = r.object();
        self->m_baseUrl->setText(d.value("baseUrl").toString());
        self->m_model->setText(d.value("model").toString());
        self->m_temperature->setValue(d.value("temperature").toDouble(0.2));
        self->m_maxTokens->setValue(d.value("maxTokens").toInt(1024));
        self->m_enabled->setChecked(d.value("enabled").toBool());
        self->m_apiKey->clear();
        const bool configured = d.value("configured").toBool();
        const QString masked = d.value("apiKeyMasked").toString();
        self->m_apiKey->setPlaceholderText(
            configured ? QStringLiteral("已配置 %1（留空＝不修改）").arg(masked)
                       : QStringLiteral("未配置，请填入 API Key"));
    });
}

QJsonObject AiConfigPage::collectBody() const {
    QJsonObject body;
    body["baseUrl"] = m_baseUrl->text().trimmed();
    body["apiKey"] = m_apiKey->text();   // 留空＝服务端保持原值
    body["model"] = m_model->text().trimmed();
    body["temperature"] = m_temperature->value();
    body["maxTokens"] = m_maxTokens->value();
    body["enabled"] = m_enabled->isChecked();
    return body;
}

void AiConfigPage::testConnection() {
    m_testBtn->setEnabled(false);
    setStatus(m_status, QStringLiteral("正在测试连接..."), 0);
    QPointer<AiConfigPage> self(this);
    ApiClient::instance().post("/ai/llm-config/test", collectBody(), [self](const ApiResponse &r) {
        if (!self) {
            return;
        }
        self->m_testBtn->setEnabled(true);
        if (!r.ok) {
            self->setStatus(self->m_status, r.message, 2);
            return;
        }
        const QJsonObject d = r.object();
        const bool ok = d.value("ok").toBool();
        const QString msg = d.value("message").toString();
        const int latency = d.value("latencyMs").toInt();
        if (ok) {
            self->setStatus(self->m_status,
                            QStringLiteral("连通成功（耗时 %1 ms）：%2").arg(latency).arg(msg), 1);
        } else {
            self->setStatus(self->m_status, QStringLiteral("连通失败：%1").arg(msg), 2);
        }
    });
}

void AiConfigPage::saveConfig() {
    m_saveBtn->setEnabled(false);
    setStatus(m_status, QStringLiteral("保存中..."), 0);
    QPointer<AiConfigPage> self(this);
    ApiClient::instance().put("/ai/llm-config", collectBody(), [self](const ApiResponse &r) {
        if (!self) {
            return;
        }
        self->m_saveBtn->setEnabled(true);
        if (!r.ok) {
            self->setStatus(self->m_status, r.message, 2);
            return;
        }
        const QJsonObject d = r.object();
        const QString warning = d.value("pushWarning").toString();
        if (!warning.isEmpty()) {
            self->setStatus(self->m_status, warning, 2);
        } else {
            self->setStatus(self->m_status, QStringLiteral("已保存并下发，问答将立即按新配置作答。"), 1);
        }
        self->loadConfig();   // 刷新掩码占位与启用状态
    });
}

void AiConfigPage::rebuildIndex() {
    m_rebuildBtn->setEnabled(false);
    setStatus(m_rebuildStatus, QStringLiteral("重建中，请稍候..."), 0);
    QPointer<AiConfigPage> self(this);
    ApiClient::instance().post("/ai/index/rebuild", QJsonObject(), [self](const ApiResponse &r) {
        if (!self) {
            return;
        }
        self->m_rebuildBtn->setEnabled(Session::instance().hasPermission(QStringLiteral("ai:index")));
        if (!r.ok) {
            self->setStatus(self->m_rebuildStatus, r.message, 2);
            return;
        }
        const QJsonObject d = r.object();
        self->setStatus(self->m_rebuildStatus,
                        QStringLiteral("重建完成：共 %1，成功 %2，空 %3，失败 %4")
                            .arg(d.value("total").toInt())
                            .arg(d.value("indexed").toInt())
                            .arg(d.value("empty").toInt())
                            .arg(d.value("failed").toInt()),
                        1);
    });
}

void AiConfigPage::setStatus(QLabel *label, const QString &text, int level) {
    if (!label) {
        return;
    }
    label->setText(text);
    const char *color = level == 1 ? "#6B7F74" : (level == 2 ? "#B94A48" : "#757575");
    label->setStyleSheet(QStringLiteral("color:%1;").arg(color));
    if (level == 2 && !text.isEmpty()) {
        notify::warn(this, text);
    }
}

} // namespace kb
