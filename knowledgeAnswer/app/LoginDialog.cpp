#include "app/LoginDialog.h"

#include "core/auth/Session.h"
#include "core/config/Settings.h"
#include "core/network/ApiClient.h"

#include <QCheckBox>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QFrame>

namespace kb {

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent) {
    setObjectName("LoginDialog");
    setWindowTitle(QStringLiteral("登录 · 智能客服知识库"));
    buildUi();
}

void LoginDialog::buildUi() {
    setFixedWidth(400);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(28, 28, 28, 28);

    auto *card = new QFrame(this);
    card->setObjectName("LoginCard");
    auto *form = new QVBoxLayout(card);
    form->setContentsMargins(28, 30, 28, 28);
    form->setSpacing(14);

    auto *brand = new QLabel(QStringLiteral("智能客服知识库"), card);
    brand->setObjectName("BrandTitle");
    auto *subtitle = new QLabel(QStringLiteral("知识中台 · 控制台"), card);
    subtitle->setObjectName("BrandSubtitle");

    m_username = new QLineEdit(card);
    m_username->setPlaceholderText(QStringLiteral("用户名"));
    m_username->setClearButtonEnabled(true);

    m_password = new QLineEdit(card);
    m_password->setPlaceholderText(QStringLiteral("密码"));
    m_password->setEchoMode(QLineEdit::Password);

    m_remember = new QCheckBox(QStringLiteral("记住账号"), card);

    m_loginBtn = new QPushButton(QStringLiteral("登 录"), card);
    m_loginBtn->setObjectName("PrimaryButton");
    m_loginBtn->setDefault(true);
    m_loginBtn->setCursor(Qt::PointingHandCursor);

    m_status = new QLabel(card);
    m_status->setObjectName("StatusLabel");
    m_status->setWordWrap(true);

    // 可展开的服务器地址设置
    m_serverToggle = new QToolButton(card);
    m_serverToggle->setObjectName("LinkButton");
    m_serverToggle->setText(QStringLiteral("服务器设置"));
    m_serverToggle->setCheckable(true);
    m_serverToggle->setCursor(Qt::PointingHandCursor);

    m_server = new QLineEdit(card);
    m_server->setPlaceholderText(QStringLiteral("服务器地址，如 http://localhost:8080/api"));
    m_server->setText(Settings::instance().baseUrl());
    m_server->setVisible(false);

    auto *hint = new QLabel(QStringLiteral("默认账号 admin / 123456"), card);
    hint->setObjectName("HintLabel");

    form->addWidget(brand);
    form->addWidget(subtitle);
    form->addSpacing(8);
    form->addWidget(m_username);
    form->addWidget(m_password);
    form->addWidget(m_remember);
    form->addSpacing(4);
    form->addWidget(m_loginBtn);
    form->addWidget(m_status);
    form->addSpacing(2);
    form->addWidget(m_serverToggle, 0, Qt::AlignLeft);
    form->addWidget(m_server);
    form->addWidget(hint, 0, Qt::AlignLeft);

    root->addWidget(card);

    // 记住的账号回填
    auto &settings = Settings::instance();
    m_remember->setChecked(settings.rememberUsername());
    if (settings.rememberUsername()) {
        m_username->setText(settings.rememberedUsername());
    }
    if (m_username->text().isEmpty()) {
        m_username->setFocus();
    } else {
        m_password->setFocus();
    }

    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLogin);
    connect(m_password, &QLineEdit::returnPressed, this, &LoginDialog::onLogin);
    connect(m_username, &QLineEdit::returnPressed, this, [this]() { m_password->setFocus(); });
    connect(m_serverToggle, &QToolButton::toggled, m_server, &QWidget::setVisible);
}

void LoginDialog::setStatus(const QString &msg) {
    m_status->setText(msg);
}

void LoginDialog::setBusy(bool busy) {
    m_loginBtn->setEnabled(!busy);
    m_loginBtn->setText(busy ? QStringLiteral("登录中…") : QStringLiteral("登 录"));
    m_username->setEnabled(!busy);
    m_password->setEnabled(!busy);
}

void LoginDialog::onLogin() {
    const QString user = m_username->text().trimmed();
    const QString pwd = m_password->text();
    if (user.isEmpty() || pwd.isEmpty()) {
        setStatus(QStringLiteral("请输入用户名和密码"));
        return;
    }
    const QString url = m_server->text().trimmed();
    if (!url.isEmpty()) {
        Settings::instance().setBaseUrl(url);
    }

    setStatus(QString());
    setBusy(true);

    QPointer<LoginDialog> self(this);
    const QJsonObject body{{"username", user}, {"password", pwd}};

    ApiClient::instance().post("/auth/login", body, [self, user](const ApiResponse &r) {
        if (!self) {
            return;
        }
        if (r.networkError) {
            self->setStatus(QStringLiteral("无法连接服务器：%1").arg(r.message));
            self->setBusy(false);
            return;
        }
        if (!r.ok) {
            self->setStatus(r.message.isEmpty() ? QStringLiteral("登录失败") : r.message);
            self->setBusy(false);
            return;
        }
        const QString token = r.object().value("token").toString();
        if (token.isEmpty()) {
            self->setStatus(QStringLiteral("登录响应缺少 token"));
            self->setBusy(false);
            return;
        }
        Session::instance().setToken(token);

        // 拿到 token 后立即获取当前用户 + 菜单 + 权限码
        ApiClient::instance().get("/auth/me", [self, user](const ApiResponse &mr) {
            if (!self) {
                return;
            }
            if (!mr.ok) {
                Session::instance().clear();
                self->setStatus(QStringLiteral("获取用户信息失败：%1").arg(mr.message));
                self->setBusy(false);
                return;
            }
            Session::instance().applyMe(mr.object());

            auto &settings = Settings::instance();
            settings.setRememberUsername(self->m_remember->isChecked());
            settings.setRememberedUsername(self->m_remember->isChecked() ? user : QString());

            self->accept();
        });
    });
}

} // namespace kb
