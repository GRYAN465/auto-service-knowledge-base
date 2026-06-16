#pragma once

#include <QObject>

namespace kb {

class MainWindow;

/**
 * 应用控制器：编排「登录 → 主窗口」流程，并处理退出登录后的重新登录。
 */
class Application : public QObject {
    Q_OBJECT
public:
    explicit Application(QObject *parent = nullptr);

    /** 启动：弹登录框，成功则显示主窗口。返回 false 表示用户取消首次登录。 */
    bool start();

private slots:
    void onLogout();

private:
    bool showLogin();

    MainWindow *m_main = nullptr;
};

} // namespace kb
