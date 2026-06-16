#include "app/Application.h"

#include "app/LoginDialog.h"
#include "app/MainWindow.h"
#include "core/auth/Session.h"

#include <QApplication>
#include <QDialog>

namespace kb {

Application::Application(QObject *parent) : QObject(parent) {}

bool Application::start() {
    return showLogin();
}

bool Application::showLogin() {
    LoginDialog dlg;
    if (dlg.exec() != QDialog::Accepted) {
        return false;
    }
    m_main = new MainWindow();
    // 队列连接：等按钮 clicked 的信号发射完整展开后再处理退出，避免在发射栈中析构主窗口
    connect(m_main, &MainWindow::loggedOut, this, &Application::onLogout, Qt::QueuedConnection);
    m_main->show();
    return true;
}

void Application::onLogout() {
    if (m_main) {
        m_main->hide();
        m_main->deleteLater();
        m_main = nullptr;
    }
    Session::instance().clear();
    if (!showLogin()) {
        QApplication::quit();
    }
}

} // namespace kb
