#pragma once

#include "app/RefreshablePage.h"

#include <QWidget>

class QLabel;
class QTableWidget;

namespace kb {

/**
 * 首页仪表盘（路由 dashboard，目录「工作台」）：登录后的落地页，按角色自适应。
 *   - 所有角色：问候区 + 快捷入口（知识检索 / 我的收藏 / 我的分享，按菜单可见性显隐）。
 *   - 有 statistics:view 的角色：额外展示概览指标卡（知识总数 / 已上线 / 待审核 / 草稿）、今日动态。
 *   - 有 knowledge:search 的角色：热门知识 TOP5（双击开只读详情），数据来自 /statistics/hot-article。
 * 无 statistics:view 的角色不发起 /statistics/overview，避免 2003。统一复用 app.qss 主题。
 */
class DashboardPage : public QWidget, public RefreshablePage {
    Q_OBJECT

public:
    explicit DashboardPage(const QString &title, QWidget *parent = nullptr);

    void refreshPage() override;

signals:
    /** 请求切换到指定路由（由 MainWindow 接到左侧导航树）。 */
    void navigateRequested(const QString &name);

private:
    void buildUi();
    void loadOverview();
    void loadHotArticles();
    void openArticleDetail();
    void setStatus(const QString &text, bool error = false);

    QString m_title;
    bool m_canStats = false;
    bool m_canHotArticles = false;

    QLabel *m_status = nullptr;
    QLabel *m_cardTotal = nullptr;
    QLabel *m_cardOnline = nullptr;
    QLabel *m_cardPending = nullptr;
    QLabel *m_cardDraft = nullptr;
    QLabel *m_todayLine = nullptr;
    QTableWidget *m_hotArticles = nullptr;
};

} // namespace kb
