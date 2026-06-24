#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QToolButton;

namespace kb {

/**
 * 登录 / 注册对话框：登录调 POST /auth/login；注册调 POST /auth/register（默认 USER 角色）。
 */
class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);

private slots:
    void onLogin();
    void onRegister();
    void showLoginPage();
    void showRegisterPage();

private:
    void buildUi();
    void buildLoginPage(QWidget *page);
    void buildRegisterPage(QWidget *page);
    void loadRegisterOrgs();
    void setLoginBusy(bool busy);
    void setRegisterBusy(bool busy);
    void setLoginStatus(const QString &msg);
    void setRegisterStatus(const QString &msg);
    void applyServerUrl();

    QStackedWidget *m_stack = nullptr;

    // 登录
    QLineEdit *m_username = nullptr;
    QLineEdit *m_password = nullptr;
    QCheckBox *m_remember = nullptr;
    QPushButton *m_loginBtn = nullptr;
    QLabel *m_loginStatus = nullptr;

    // 注册
    QLineEdit *m_regUsername = nullptr;
    QLineEdit *m_regPassword = nullptr;
    QLineEdit *m_regConfirm = nullptr;
    QLineEdit *m_regRealName = nullptr;
    QComboBox *m_regOrg = nullptr;
    QLineEdit *m_regPhone = nullptr;
    QLineEdit *m_regEmail = nullptr;
    QComboBox *m_regGender = nullptr;
    QPushButton *m_registerBtn = nullptr;
    QLabel *m_registerStatus = nullptr;
    bool m_orgsLoaded = false;

    QLineEdit *m_server = nullptr;
    QToolButton *m_serverToggle = nullptr;
};

} // namespace kb
