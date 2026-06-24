#pragma once

#include "app/RefreshablePage.h"

#include <QWidget>

class QLabel;
class QTableWidget;

namespace kb {

/**
 * 首页仪表盘（路由 dashboard，目录「工作台」）：登录后的落地页，按角色自适应。
 *   - 所有角色：问候区 + 快捷入口。
 *   - statistics:view：概览指标卡 + 今日动态。
 *   - knowledge:search：推荐知识 TOP5（/knowledge/recommend/home）+ 热门 TOP5。
 */
class DashboardPage : public QWidget, public RefreshablePage {
    Q_OBJECT

public:
    explicit DashboardPage(const QString &title, QWidget *parent = nullptr);

    void refreshPage() override;

signals:
    void navigateRequested(const QString &name);

private:
    void buildUi();
    void loadOverview();
    void loadRecommendations();
    void loadHotArticles();
    void openArticleDetail(QTableWidget *table);
    void setStatus(const QString &text, bool error = false);
    static QString pinnedTagIdsParam();

    QString m_title;
    bool m_canStats = false;
    bool m_canHotArticles = false;

    QLabel *m_status = nullptr;
    QLabel *m_recommendHint = nullptr;
    QLabel *m_cardTotal = nullptr;
    QLabel *m_cardOnline = nullptr;
    QLabel *m_cardPending = nullptr;
    QLabel *m_cardDraft = nullptr;
    QLabel *m_todayLine = nullptr;
    QTableWidget *m_recommendArticles = nullptr;
    QTableWidget *m_hotArticles = nullptr;
};

} // namespace kb
