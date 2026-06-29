#pragma once



#include "app/RefreshablePage.h"



#include <QFrame>

#include <QWidget>



class QComboBox;

class QLabel;



namespace kb {



class ChartCard;

struct OverviewKpiLabels {
    QLabel *articleTotal = nullptr;
    QLabel *articleOnline = nullptr;
    QLabel *articlePending = nullptr;
    QLabel *articleDraft = nullptr;
    QLabel *articleOffline = nullptr;
    QLabel *viewTotal = nullptr;
    QLabel *favoriteTotal = nullptr;
    QLabel *likeTotal = nullptr;
    QLabel *commentTotal = nullptr;
    QLabel *categoryTag = nullptr;
    QLabel *today = nullptr;
};



class StatisticsPage : public QWidget, public RefreshablePage {

    Q_OBJECT



public:

    explicit StatisticsPage(const QString &title, QWidget *parent = nullptr);



    void refreshPage() override;



private:

    void buildUi();

    void refresh();

    void updatePeriodControls();

    int trendDays() const;

    int trendYear() const;

    bool isMonthlyTrend() const;



    void loadOverview();

    void loadTrend();

    void loadCategoryRank();

    void loadHotArticles();

    void loadHotKeywords();

    void loadAuditOverview();

    void loadQaOverview();

    void setStatus(const QString &text, bool error = false);



    QString m_title;

    QLabel *m_status = nullptr;

    QComboBox *m_periodWindow = nullptr;

    QComboBox *m_yearWindow = nullptr;

    QLabel *m_yearLabel = nullptr;



    QFrame *m_kpiSection = nullptr;
    OverviewKpiLabels m_kpi;



    ChartCard *m_activityChart = nullptr;

    ChartCard *m_contentChart = nullptr;

    ChartCard *m_statusPie = nullptr;

    ChartCard *m_typePie = nullptr;

    ChartCard *m_categoryChart = nullptr;

    ChartCard *m_hotArticleChart = nullptr;

    ChartCard *m_hotKeywordChart = nullptr;

    ChartCard *m_auditChart = nullptr;

    ChartCard *m_qaChart = nullptr;

};



} // namespace kb

