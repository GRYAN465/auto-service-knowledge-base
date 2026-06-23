#include "app/ChangePasswordDialog.h"

#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace kb {

ChangePasswordDialog::ChangePasswordDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("修改密码"));
    resize(420, 220);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    auto *hint = new QLabel(QStringLiteral("请输入原密码与新密码（至少 6 位）。"), this);
    hint->setObjectName("HintLabel");
    root->addWidget(hint);

    auto *card = new QFrame(this);
    card->setObjectName("SectionCard");
    auto *form = new QVBoxLayout(card);
    form->setContentsMargins(16, 16, 16, 16);
    form->setSpacing(10);

    m_oldPwd = new QLineEdit(card);
    m_oldPwd->setEchoMode(QLineEdit::Password);
    m_oldPwd->setPlaceholderText(QStringLiteral("原密码"));
    m_newPwd = new QLineEdit(card);
    m_newPwd->setEchoMode(QLineEdit::Password);
    m_newPwd->setPlaceholderText(QStringLiteral("新密码"));
    form->addWidget(new QLabel(QStringLiteral("原密码"), card));
    form->addWidget(m_oldPwd);
    form->addWidget(new QLabel(QStringLiteral("新密码"), card));
    form->addWidget(m_newPwd);
    root->addWidget(card);

    auto *footer = new QHBoxLayout();
    footer->addStretch();
    auto *cancel = new QPushButton(QStringLiteral("取消"), this);
    cancel->setObjectName("GhostButton");
    auto *ok = new QPushButton(QStringLiteral("确认"), this);
    ok->setObjectName("PrimaryButton");
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(ok, &QPushButton::clicked, this, [this]() {
        QJsonObject body;
        body.insert("oldPassword", m_oldPwd->text());
        body.insert("newPassword", m_newPwd->text());
        ApiClient::instance().put("/auth/me/password", body, [this](const ApiResponse &r) {
            if (!r.ok) {
                notify::warn(this, r.message);
                return;
            }
            notify::info(this, QStringLiteral("密码修改成功"));
            accept();
        });
    });
    footer->addWidget(cancel);
    footer->addWidget(ok);
    root->addLayout(footer);
}

} // namespace kb
