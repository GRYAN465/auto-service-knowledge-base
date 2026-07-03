#include "app/LoginDialog.h"

#include "core/auth/Session.h"
#include "core/config/Settings.h"
#include "core/network/ApiClient.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace kb {

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent) {
    setObjectName("LoginDialog");
    setWindowTitle(QStringLiteral("登录 · 坐席智能体"));
    buildUi();
}

void LoginDialog::buildUi() {
    setFixedWidth(420);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(28, 28, 28, 28);

    auto *card = new QFrame(this);
    card->setObjectName("LoginCard");
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(28, 30, 28, 28);
    cardLayout->setSpacing(12);

    m_stack = new QStackedWidget(card);
    auto *loginPage = new QWidget(card);
    auto *registerPage = new QWidget(card);
    buildLoginPage(loginPage);
    buildRegisterPage(registerPage);
    m_stack->addWidget(loginPage);
    m_stack->addWidget(registerPage);
    cardLayout->addWidget(m_stack);

    m_serverToggle = new QToolButton(card);
    m_serverToggle->setObjectName("LinkButton");
    m_serverToggle->setText(QStringLiteral("服务器设置"));
    m_serverToggle->setCheckable(true);
    m_serverToggle->setCursor(Qt::PointingHandCursor);

    m_server = new QLineEdit(card);
    m_server->setPlaceholderText(QStringLiteral("服务器地址，如 http://localhost:8080/api"));
    m_server->setText(Settings::instance().baseUrl());
    m_server->setVisible(false);

    cardLayout->addSpacing(4);
    cardLayout->addWidget(m_serverToggle, 0, Qt::AlignLeft);
    cardLayout->addWidget(m_server);

    root->addWidget(card);

    connect(m_serverToggle, &QToolButton::toggled, m_server, &QWidget::setVisible);
    m_stack->setCurrentIndex(0);
}

void LoginDialog::buildLoginPage(QWidget *page) {
    auto *form = new QVBoxLayout(page);
    form->setContentsMargins(0, 0, 0, 0);
    form->setSpacing(14);

    auto *brandRow = new QWidget(page);
    brandRow->setObjectName("LoginBrandRow");
    auto *brandRowLayout = new QHBoxLayout(brandRow);
    brandRowLayout->setContentsMargins(0, 0, 0, 0);
    brandRowLayout->setSpacing(12);

    auto *brandLogo = new QLabel(brandRow);
    brandLogo->setObjectName("AppBrandMark");
    brandLogo->setPixmap(QIcon(QStringLiteral(":/icons/app-logo-mark.svg")).pixmap(44, 44));
    brandLogo->setFixedSize(44, 44);
    brandLogo->setScaledContents(true);

    auto *brandText = new QWidget(brandRow);
    auto *brandTextLayout = new QVBoxLayout(brandText);
    brandTextLayout->setContentsMargins(0, 0, 0, 0);
    brandTextLayout->setSpacing(2);

    auto *brand = new QLabel(QStringLiteral("坐席智能体"), brandText);
    brand->setObjectName("BrandTitle");
    auto *subtitle = new QLabel(QStringLiteral("智能客服知识库 · 控制台"), brandText);
    subtitle->setObjectName("BrandSubtitle");

    brandTextLayout->addWidget(brand);
    brandTextLayout->addWidget(subtitle);
    brandRowLayout->addWidget(brandLogo);
    brandRowLayout->addWidget(brandText, 1);
    form->addWidget(brandRow);

    m_username = new QLineEdit(page);
    m_username->setPlaceholderText(QStringLiteral("用户名"));
    m_username->setClearButtonEnabled(true);

    m_password = new QLineEdit(page);
    m_password->setPlaceholderText(QStringLiteral("密码"));
    m_password->setEchoMode(QLineEdit::Password);

    m_remember = new QCheckBox(QStringLiteral("记住账号"), page);

    m_loginBtn = new QPushButton(QStringLiteral("登 录"), page);
    m_loginBtn->setObjectName("PrimaryButton");
    m_loginBtn->setDefault(true);
    m_loginBtn->setCursor(Qt::PointingHandCursor);

    m_loginStatus = new QLabel(page);
    m_loginStatus->setObjectName("StatusLabel");
    m_loginStatus->setWordWrap(true);

    auto *toRegister = new QToolButton(page);
    toRegister->setObjectName("LinkButton");
    toRegister->setText(QStringLiteral("没有账号？去注册"));
    toRegister->setCursor(Qt::PointingHandCursor);

    auto *hint = new QLabel(QStringLiteral("演示账号 admin / 123456"), page);
    hint->setObjectName("HintLabel");

    form->addSpacing(8);
    form->addWidget(m_username);
    form->addWidget(m_password);
    form->addWidget(m_remember);
    form->addSpacing(4);
    form->addWidget(m_loginBtn);
    form->addWidget(m_loginStatus);
    form->addWidget(toRegister, 0, Qt::AlignLeft);
    form->addWidget(hint, 0, Qt::AlignLeft);

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
    connect(toRegister, &QToolButton::clicked, this, &LoginDialog::showRegisterPage);
}

void LoginDialog::buildRegisterPage(QWidget *page) {
    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(10);

    auto *brandRow = new QWidget(page);
    brandRow->setObjectName("LoginBrandRow");
    auto *brandRowLayout = new QHBoxLayout(brandRow);
    brandRowLayout->setContentsMargins(0, 0, 0, 0);
    brandRowLayout->setSpacing(12);

    auto *brandLogo = new QLabel(brandRow);
    brandLogo->setObjectName("AppBrandMark");
    brandLogo->setPixmap(QIcon(QStringLiteral(":/icons/app-logo-mark.svg")).pixmap(40, 40));
    brandLogo->setFixedSize(40, 40);
    brandLogo->setScaledContents(true);

    auto *brandText = new QWidget(brandRow);
    auto *brandTextLayout = new QVBoxLayout(brandText);
    brandTextLayout->setContentsMargins(0, 0, 0, 0);
    brandTextLayout->setSpacing(2);

    auto *brand = new QLabel(QStringLiteral("注册账号"), brandText);
    brand->setObjectName("BrandTitle");
    auto *subtitle = new QLabel(QStringLiteral("坐席智能体 · 注册后默认为普通用户"), brandText);
    subtitle->setObjectName("BrandSubtitle");

    brandTextLayout->addWidget(brand);
    brandTextLayout->addWidget(subtitle);
    brandRowLayout->addWidget(brandLogo);
    brandRowLayout->addWidget(brandText, 1);
    outer->addWidget(brandRow);

    auto *scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *scrollHost = new QWidget(scroll);
    auto *form = new QFormLayout(scrollHost);
    form->setContentsMargins(0, 0, 4, 0);
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_regUsername = new QLineEdit(scrollHost);
    m_regUsername->setPlaceholderText(QStringLiteral("3~64 位字母数字"));
    m_regUsername->setClearButtonEnabled(true);

    m_regPassword = new QLineEdit(scrollHost);
    m_regPassword->setPlaceholderText(QStringLiteral("至少 6 位"));
    m_regPassword->setEchoMode(QLineEdit::Password);

    m_regConfirm = new QLineEdit(scrollHost);
    m_regConfirm->setPlaceholderText(QStringLiteral("再次输入密码"));
    m_regConfirm->setEchoMode(QLineEdit::Password);

    m_regRealName = new QLineEdit(scrollHost);
    m_regRealName->setPlaceholderText(QStringLiteral("真实姓名"));

    m_regOrg = new QComboBox(scrollHost);
    m_regOrg->addItem(QStringLiteral("请选择机构"), QVariant::fromValue<qint64>(0));

    m_regPhone = new QLineEdit(scrollHost);
    m_regPhone->setPlaceholderText(QStringLiteral("可选"));

    m_regEmail = new QLineEdit(scrollHost);
    m_regEmail->setPlaceholderText(QStringLiteral("可选"));

    m_regGender = new QComboBox(scrollHost);
    m_regGender->addItem(QStringLiteral("未指定"), QStringLiteral("U"));
    m_regGender->addItem(QStringLiteral("男"), QStringLiteral("M"));
    m_regGender->addItem(QStringLiteral("女"), QStringLiteral("F"));

    form->addRow(QStringLiteral("用户名"), m_regUsername);
    form->addRow(QStringLiteral("密码"), m_regPassword);
    form->addRow(QStringLiteral("确认密码"), m_regConfirm);
    form->addRow(QStringLiteral("姓名"), m_regRealName);
    form->addRow(QStringLiteral("机构"), m_regOrg);
    form->addRow(QStringLiteral("手机"), m_regPhone);
    form->addRow(QStringLiteral("邮箱"), m_regEmail);
    form->addRow(QStringLiteral("性别"), m_regGender);

    scroll->setWidget(scrollHost);
    scroll->setMaximumHeight(320);
    outer->addWidget(scroll, 1);

    m_registerBtn = new QPushButton(QStringLiteral("注 册"), page);
    m_registerBtn->setObjectName("PrimaryButton");
    m_registerBtn->setCursor(Qt::PointingHandCursor);

    m_registerStatus = new QLabel(page);
    m_registerStatus->setObjectName("StatusLabel");
    m_registerStatus->setWordWrap(true);

    auto *toLogin = new QToolButton(page);
    toLogin->setObjectName("LinkButton");
    toLogin->setText(QStringLiteral("已有账号？去登录"));
    toLogin->setCursor(Qt::PointingHandCursor);

    outer->addWidget(m_registerBtn);
    outer->addWidget(m_registerStatus);
    outer->addWidget(toLogin, 0, Qt::AlignLeft);

    connect(m_registerBtn, &QPushButton::clicked, this, &LoginDialog::onRegister);
    connect(m_regConfirm, &QLineEdit::returnPressed, this, &LoginDialog::onRegister);
    connect(toLogin, &QToolButton::clicked, this, &LoginDialog::showLoginPage);
}

void LoginDialog::applyServerUrl() {
    const QString url = m_server->text().trimmed();
    if (!url.isEmpty()) {
        Settings::instance().setBaseUrl(url);
    }
}

void LoginDialog::showLoginPage() {
    m_stack->setCurrentIndex(0);
    setWindowTitle(QStringLiteral("登录 · 坐席智能体"));
}

void LoginDialog::showRegisterPage() {
    applyServerUrl();
    m_stack->setCurrentIndex(1);
    setWindowTitle(QStringLiteral("注册 · 坐席智能体"));
    setRegisterStatus(QString());
    if (!m_orgsLoaded) {
        loadRegisterOrgs();
    }
    m_regUsername->setFocus();
}

void LoginDialog::loadRegisterOrgs() {
    m_regOrg->clear();
    m_regOrg->addItem(QStringLiteral("加载机构…"), QVariant::fromValue<qint64>(0));
    m_regOrg->setEnabled(false);

    QPointer<LoginDialog> self(this);
    ApiClient::instance().get("/auth/register/orgs", [this, self](const ApiResponse &r) {
        if (!self) {
            return;
        }
        m_regOrg->clear();
        m_regOrg->addItem(QStringLiteral("请选择机构"), QVariant::fromValue<qint64>(0));
        m_regOrg->setEnabled(true);
        if (!r.ok) {
            m_regOrg->addItem(QStringLiteral("（加载失败）"), QVariant::fromValue<qint64>(0));
            setRegisterStatus(r.message);
            return;
        }
        const QJsonArray list = r.data.toArray();
        if (list.isEmpty()) {
            m_regOrg->addItem(QStringLiteral("（暂无可用机构）"), QVariant::fromValue<qint64>(0));
            return;
        }
        for (const QJsonValue &v : list) {
            const QJsonObject o = v.toObject();
            m_regOrg->addItem(o.value("label").toString(),
                              QVariant::fromValue<qint64>(static_cast<qint64>(o.value("id").toDouble())));
        }
        m_orgsLoaded = true;
    });
}

void LoginDialog::setLoginStatus(const QString &msg) {
    m_loginStatus->setText(msg);
}

void LoginDialog::setRegisterStatus(const QString &msg) {
    m_registerStatus->setText(msg);
}

void LoginDialog::setLoginBusy(bool busy) {
    m_loginBtn->setEnabled(!busy);
    m_loginBtn->setText(busy ? QStringLiteral("登录中…") : QStringLiteral("登 录"));
    m_username->setEnabled(!busy);
    m_password->setEnabled(!busy);
}

void LoginDialog::setRegisterBusy(bool busy) {
    m_registerBtn->setEnabled(!busy);
    m_registerBtn->setText(busy ? QStringLiteral("提交中…") : QStringLiteral("注 册"));
    m_regUsername->setEnabled(!busy);
    m_regPassword->setEnabled(!busy);
    m_regConfirm->setEnabled(!busy);
    m_regRealName->setEnabled(!busy);
    m_regOrg->setEnabled(!busy);
    m_regPhone->setEnabled(!busy);
    m_regEmail->setEnabled(!busy);
    m_regGender->setEnabled(!busy);
}

void LoginDialog::onLogin() {
    const QString user = m_username->text().trimmed();
    const QString pwd = m_password->text();
    if (user.isEmpty() || pwd.isEmpty()) {
        setLoginStatus(QStringLiteral("请输入用户名和密码"));
        return;
    }
    applyServerUrl();
    setLoginStatus(QString());
    setLoginBusy(true);

    QPointer<LoginDialog> self(this);
    const QJsonObject body{{"username", user}, {"password", pwd}};

    ApiClient::instance().post("/auth/login", body, [self, user](const ApiResponse &r) {
        if (!self) {
            return;
        }
        if (r.networkError) {
            self->setLoginStatus(QStringLiteral("无法连接服务器：%1").arg(r.message));
            self->setLoginBusy(false);
            return;
        }
        if (!r.ok) {
            self->setLoginStatus(r.message.isEmpty() ? QStringLiteral("登录失败") : r.message);
            self->setLoginBusy(false);
            return;
        }
        const QString token = r.object().value("token").toString();
        if (token.isEmpty()) {
            self->setLoginStatus(QStringLiteral("登录响应缺少 token"));
            self->setLoginBusy(false);
            return;
        }
        Session::instance().setToken(token);

        ApiClient::instance().get("/auth/me", [self, user](const ApiResponse &mr) {
            if (!self) {
                return;
            }
            if (!mr.ok) {
                Session::instance().clear();
                self->setLoginStatus(QStringLiteral("获取用户信息失败：%1").arg(mr.message));
                self->setLoginBusy(false);
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

void LoginDialog::onRegister() {
    const QString username = m_regUsername->text().trimmed();
    const QString password = m_regPassword->text();
    const QString confirm = m_regConfirm->text();
    const QString realName = m_regRealName->text().trimmed();
    const qint64 orgId = m_regOrg->currentData().toLongLong();

    if (username.length() < 3) {
        setRegisterStatus(QStringLiteral("用户名至少 3 位"));
        return;
    }
    if (password.length() < 6) {
        setRegisterStatus(QStringLiteral("密码至少 6 位"));
        return;
    }
    if (password != confirm) {
        setRegisterStatus(QStringLiteral("两次输入的密码不一致"));
        return;
    }
    if (realName.isEmpty()) {
        setRegisterStatus(QStringLiteral("请填写姓名"));
        return;
    }
    if (orgId <= 0) {
        setRegisterStatus(QStringLiteral("请选择所属机构"));
        return;
    }

    applyServerUrl();

    QJsonObject body;
    body["username"] = username;
    body["password"] = password;
    body["realName"] = realName;
    body["orgId"] = orgId;
    body["gender"] = m_regGender->currentData().toString();
    const QString phone = m_regPhone->text().trimmed();
    const QString email = m_regEmail->text().trimmed();
    if (!phone.isEmpty()) {
        body["phone"] = phone;
    }
    if (!email.isEmpty()) {
        body["email"] = email;
    }

    setRegisterStatus(QString());
    setRegisterBusy(true);

    QPointer<LoginDialog> self(this);
    ApiClient::instance().post("/auth/register", body, [self, username](const ApiResponse &r) {
        if (!self) {
            return;
        }
        self->setRegisterBusy(false);
        if (r.networkError) {
            self->setRegisterStatus(QStringLiteral("无法连接服务器：%1").arg(r.message));
            return;
        }
        if (!r.ok) {
            self->setRegisterStatus(r.message.isEmpty() ? QStringLiteral("注册失败") : r.message);
            return;
        }
        self->m_username->setText(username);
        self->m_password->clear();
        self->setLoginStatus(QStringLiteral("注册成功，请登录"));
        self->showLoginPage();
    });
}

} // namespace kb
