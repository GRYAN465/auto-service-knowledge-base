#pragma once

#include <QMainWindow>

class QLabel;
class QTreeWidget;
class QTreeWidgetItem;
class QStackedWidget;

namespace kb {

class PageRouter;

/**
 * 主窗口：顶栏（页面标题 + 用户 + 退出）/ 左侧导航树（按 /auth/me 菜单构建）/
 * 中央 QStackedWidget（PageRouter 懒加载页面）。
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

signals:
    void loggedOut();

private:
    void buildUi();
    void registerPages();
    void buildNav();
    void navigateToCurrent();
    void navigateToRoute(const QString &name);

    QTreeWidget *m_nav = nullptr;
    QStackedWidget *m_stack = nullptr;
    PageRouter *m_router = nullptr;
    QLabel *m_pageTitle = nullptr;
};

} // namespace kb
