#pragma once

#include <QDialog>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;

namespace kb {

/**
 * 登录对话框：调 POST /auth/login 拿 JWT，再 GET /auth/me 填充会话，成功后 accept()。
 * 支持记住账号、可展开的服务器地址设置。
 */
class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);

private slots:
    void onLogin();

private:
    void buildUi();
    void setBusy(bool busy);
    void setStatus(const QString &msg);

    QLineEdit *m_username = nullptr;
    QLineEdit *m_password = nullptr;
    QLineEdit *m_server = nullptr;
    QCheckBox *m_remember = nullptr;
    QPushButton *m_loginBtn = nullptr;
    QLabel *m_status = nullptr;
    QToolButton *m_serverToggle = nullptr;
};

} // namespace kb
