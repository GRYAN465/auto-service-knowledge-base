#pragma once

#include <QMainWindow>

class QLabel;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QStackedWidget;

namespace kb {

class PageRouter;

/**
 * 主窗口：顶栏（页面标题 + 用户 + 退出）/ 左侧导航（品牌 + 菜单树）/
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
    void updateTopBarRefresh();
    void updatePageTitle(QTreeWidgetItem *item);

    QTreeWidget *m_nav = nullptr;
    QStackedWidget *m_stack = nullptr;
    PageRouter *m_router = nullptr;
    QLabel *m_pageTitleIcon = nullptr;
    QLabel *m_pageTitle = nullptr;
    QPushButton *m_refreshBtn = nullptr;
};

} // namespace kb
