#pragma once

#include <QWidget>

class QComboBox;
class QLabel;
class QTableWidget;

namespace kb {

/**
 * 数据统计页（模块 7，路由 statistics，权限 statistics:view）：
 *   - GET /statistics/overview  仪表盘汇总 → 指标卡（知识规模 / 互动总量）+「今日」「规模分布」两行说明
 *   - GET /statistics/hot-article 热门知识 TOP（按累计浏览量），双击行打开只读详情弹窗
 *   - GET /statistics/hot-keyword 热门搜索词（可切换近 7/30/90 天 / 全部时间窗）
 * 纯只读看板；统一复用 app.qss 主题（StatCard / DataTable / SectionTitle / MutedLine），不引入图表依赖。
 */
class StatisticsPage : public QWidget {
    Q_OBJECT

public:
    explicit StatisticsPage(const QString &title, QWidget *parent = nullptr);

private:
    void buildUi();
    void refresh();
    void loadOverview();
    void loadHotArticles();
    void loadHotKeywords();
    void openArticleDetail();
    void setStatus(const QString &text, bool error = false);

    QString m_title;
    QLabel *m_status = nullptr;

    // 概览指标卡的数值标签
    QLabel *m_cardArticleTotal = nullptr;
    QLabel *m_cardOnline = nullptr;
    QLabel *m_cardPending = nullptr;
    QLabel *m_cardDraft = nullptr;
    QLabel *m_cardViews = nullptr;
    QLabel *m_cardFavorites = nullptr;
    QLabel *m_cardLikes = nullptr;
    QLabel *m_cardComments = nullptr;
    QLabel *m_todayLine = nullptr;
    QLabel *m_scaleLine = nullptr;

    QComboBox *m_keywordWindow = nullptr;
    QTableWidget *m_hotArticles = nullptr;
    QTableWidget *m_hotKeywords = nullptr;
};

} // namespace kb
