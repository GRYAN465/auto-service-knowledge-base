#pragma once

#include "app/RefreshablePage.h"

#include <QResizeEvent>
#include <QWidget>

class QFrame;
class QLabel;
class QHBoxLayout;
class QScrollArea;
class QVBoxLayout;

namespace kb {

/**
 * 首页仪表盘（路由 dashboard）：问候 + 快捷入口 + 与我相关 KPI + 推荐/热门双栏。
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
    void loadMyKpi();
    void loadRecommendations();
    void loadHotArticles();
    void openArticle(qint64 articleId);
    void setStatus(const QString &text, bool error = false);
    void syncHotPanelHeight();
    static QString pinnedTagIdsParam();

    void resizeEvent(QResizeEvent *event) override;

    QString m_title;
    bool m_canStats = false;
    bool m_canHotArticles = false;
    bool m_kpiAdminMode = false;

    QLabel *m_status = nullptr;
    QLabel *m_kpiSectionTitle = nullptr;
    QLabel *m_kpiValues[4] = {};
    QLabel *m_kpiCaptions[4] = {};
    QLabel *m_recommendHint = nullptr;
    QScrollArea *m_quickScroll = nullptr;
    QWidget *m_quickHost = nullptr;
    QHBoxLayout *m_quickRow = nullptr;
    QVBoxLayout *m_recommendFeed = nullptr;
    QVBoxLayout *m_hotRankList = nullptr;
    QWidget *m_leftCol = nullptr;
    QFrame *m_rightCol = nullptr;
    QScrollArea *m_hotScroll = nullptr;
    QWidget *m_hotListHost = nullptr;
    QWidget *m_contentRow = nullptr;

    static constexpr int kHotFetchLimit = 15;
};

} // namespace kb
