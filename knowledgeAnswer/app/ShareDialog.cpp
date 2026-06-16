#include "app/ShareDialog.h"

#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

namespace kb {

ShareDialog::ShareDialog(qint64 articleId, QWidget *parent)
    : QDialog(parent), m_articleId(articleId) {
    setWindowTitle(QStringLiteral("分享知识"));
    resize(420, 320);
    buildUi();
    loadUsers();
}

void ShareDialog::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(10);

    root->addWidget(new QLabel(QStringLiteral("分享给同事"), this));
    m_recipient = new QComboBox(this);
    m_recipient->addItem(QStringLiteral("加载中..."), QVariant::fromValue<qint64>(0));
    m_recipient->setEnabled(false);
    root->addWidget(m_recipient);

    root->addWidget(new QLabel(QStringLiteral("留言（可选）"), this));
    m_message = new QTextEdit(this);
    m_message->setObjectName("DataTable");
    m_message->setPlaceholderText(QStringLiteral("写点推荐理由……"));
    root->addWidget(m_message, 1);

    auto *footer = new QHBoxLayout();
    footer->addStretch();
    auto *cancel = new QPushButton(QStringLiteral("取消"), this);
    cancel->setObjectName("GhostButton");
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    auto *send = new QPushButton(QStringLiteral("分享"), this);
    send->setObjectName("PrimaryButton");
    connect(send, &QPushButton::clicked, this, &ShareDialog::submit);
    footer->addWidget(cancel);
    footer->addWidget(send);
    root->addLayout(footer);
}

void ShareDialog::loadUsers() {
    ApiClient::instance().get("/interaction/users", [this](const ApiResponse &r) {
        m_recipient->clear();
        if (!r.ok) {
            m_recipient->addItem(QStringLiteral("加载失败"), QVariant::fromValue<qint64>(0));
            notify::warn(this, r.message);
            return;
        }
        const QJsonArray list = r.data.toArray();
        if (list.isEmpty()) {
            m_recipient->addItem(QStringLiteral("无可选收件人"), QVariant::fromValue<qint64>(0));
            return;
        }
        for (const QJsonValue &v : list) {
            const QJsonObject o = v.toObject();
            m_recipient->addItem(o.value("realName").toString(),
                                 QVariant::fromValue<qint64>(static_cast<qint64>(o.value("id").toDouble())));
        }
        m_recipient->setEnabled(true);
    });
}

void ShareDialog::submit() {
    const qint64 toUserId = m_recipient->currentData().toLongLong();
    if (toUserId <= 0) {
        notify::warn(this, QStringLiteral("请选择收件人"));
        return;
    }
    QJsonObject body;
    body.insert("articleId", m_articleId);
    body.insert("toUserId", toUserId);
    body.insert("message", m_message->toPlainText().trimmed());
    ApiClient::instance().post("/interaction/share", body, [this](const ApiResponse &r) {
        if (!r.ok) {
            notify::warn(this, r.message);
            return;
        }
        notify::info(this, QStringLiteral("已分享"));
        accept();
    });
}

} // namespace kb
