#include "app/StatisticsPage.h"

#include "app/RefreshablePage.h"
#include "common/ChartCard.h"
#include "common/ChartTheme.h"
#include "common/ComboStyle.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QChart>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QDate>
#include <QtMath>
#include <QScrollArea>
#include <QVBoxLayout>

namespace kb {

namespace {

qint64 asLong(const QJsonValue &v) {
    return static_cast<qint64>(v.toDouble());
}

QString truncateLabel(const QString &text, int maxLen = 14) {
    const QString t = text.trimmed();
    if (t.length() <= maxLen) {
        return t;
    }
    return t.left(maxLen) + QStringLiteral("…");
}

QStringList datesFromTrend(const QJsonArray &arr, bool monthly) {
    QStringList out;
    for (const QJsonValue &v : arr) {
        const QString d = v.toObject().value("date").toString();
        if (monthly && d.length() >= 7) {
            const int month = d.mid(5, 2).toInt();
            out << QStringLiteral("%1月").arg(month);
        } else if (d.length() >= 10) {
            out << d.mid(5);
        } else {
            out << d;
        }
    }
    return out;
}

QVector<qint64> countsFromTrend(const QJsonArray &arr) {
    QVector<qint64> out;
    out.reserve(arr.size());
    for (const QJsonValue &v : arr) {
        out << asLong(v.toObject().value("count"));
    }
    return out;
}

QFrame *makeKpiTile(const QString &caption, QLabel *&valueOut, QWidget *parent, bool accent = false) {
    auto *tile = new QFrame(parent);
    tile->setObjectName(QStringLiteral("KpiTile"));
    auto *lay = new QVBoxLayout(tile);
    lay->setContentsMargins(14, 12, 14, 12);
    lay->setSpacing(4);
    valueOut = new QLabel(QStringLiteral("—"), tile);
    valueOut->setObjectName(accent ? QStringLiteral("KpiTileValueAccent") : QStringLiteral("KpiTileValue"));
    auto *cap = new QLabel(caption, tile);
    cap->setObjectName(QStringLiteral("KpiTileLabel"));
    lay->addWidget(valueOut);
    lay->addWidget(cap);
    return tile;
}

QFrame *makeKpiSection(QWidget *parent, OverviewKpiLabels &kpi, QHBoxLayout *titleRight) {
    auto *section = new QFrame(parent);
    section->setObjectName("SectionCard");
    auto *layout = new QVBoxLayout(section);
    layout->setContentsMargins(20, 18, 20, 16);
    layout->setSpacing(12);
    auto *titleRow = new QHBoxLayout();
    titleRow->setSpacing(8);
    auto *title = new QLabel(QStringLiteral("运营总览"), section);
    title->setObjectName("SectionTitle");
    titleRow->addWidget(title);
    titleRow->addStretch();
    if (titleRight) {
        while (QLayoutItem *item = titleRight->takeAt(0)) {
            titleRow->addItem(item);
        }
        delete titleRight;
    }
    layout->addLayout(titleRow);

    auto *grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(10);

    grid->addWidget(makeKpiTile(QStringLiteral("知识总数"), kpi.articleTotal, section), 0, 0);
    grid->addWidget(makeKpiTile(QStringLiteral("已上线"), kpi.articleOnline, section, true), 0, 1);
    grid->addWidget(makeKpiTile(QStringLiteral("待审核"), kpi.articlePending, section), 0, 2);
    grid->addWidget(makeKpiTile(QStringLiteral("草稿"), kpi.articleDraft, section), 0, 3);
    grid->addWidget(makeKpiTile(QStringLiteral("已下线"), kpi.articleOffline, section), 0, 4);
    grid->addWidget(makeKpiTile(QStringLiteral("累计浏览"), kpi.viewTotal, section), 1, 0);
    grid->addWidget(makeKpiTile(QStringLiteral("累计收藏"), kpi.favoriteTotal, section), 1, 1);
    grid->addWidget(makeKpiTile(QStringLiteral("累计点赞"), kpi.likeTotal, section), 1, 2);
    grid->addWidget(makeKpiTile(QStringLiteral("累计评论"), kpi.commentTotal, section), 1, 3);
    grid->addWidget(makeKpiTile(QStringLiteral("分类 / 标签"), kpi.categoryTag, section), 1, 4);

    layout->addLayout(grid);

    kpi.today = new QLabel(section);
    kpi.today->setObjectName(QStringLiteral("KpiTodayStrip"));
    kpi.today->setTextFormat(Qt::RichText);
    layout->addWidget(kpi.today);
    return section;
}

} // namespace

StatisticsPage::StatisticsPage(const QString &title, QWidget *parent)
    : QWidget(parent), m_title(title) {
    buildUi();
    refresh();
}

void StatisticsPage::buildUi() {
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *content = new QWidget(scroll);
    auto *root = new QVBoxLayout(content);
    root->setContentsMargins(24, 16, 24, 28);
    root->setSpacing(16);

    m_periodWindow = new QComboBox(content);
    m_periodWindow->addItem(QStringLiteral("近 7 天"), 7);
    m_periodWindow->addItem(QStringLiteral("近 30 天"), 30);
    m_periodWindow->addItem(QStringLiteral("近 90 天"), 90);
    m_periodWindow->addItem(QStringLiteral("按年"), -1);
    m_periodWindow->setCurrentIndex(0);
    ComboStyle::polish(m_periodWindow);

    m_yearLabel = new QLabel(QStringLiteral("年份"), content);
    m_yearWindow = new QComboBox(content);
    const int curYear = QDate::currentDate().year();
    for (int y = curYear; y >= curYear - 3; --y) {
        m_yearWindow->addItem(QString::number(y), y);
    }
    ComboStyle::polish(m_yearWindow);

    connect(m_periodWindow, &QComboBox::currentIndexChanged, this, [this]() {
        updatePeriodControls();
        loadTrend();
        loadHotKeywords();
        loadQaOverview();
    });
    connect(m_yearWindow, &QComboBox::currentIndexChanged, this, [this]() {
        if (isMonthlyTrend()) {
            loadTrend();
        }
    });

    auto *periodBar = new QHBoxLayout();
    periodBar->setSpacing(8);
    auto *periodLabel = new QLabel(QStringLiteral("统计周期"), content);
    periodLabel->setObjectName(QStringLiteral("MutedLabel"));
    periodBar->addWidget(periodLabel);
    periodBar->addWidget(m_periodWindow);
    periodBar->addWidget(m_yearLabel);
    periodBar->addWidget(m_yearWindow);

    m_kpiSection = makeKpiSection(content, m_kpi, periodBar);
    root->addWidget(m_kpiSection);
    updatePeriodControls();

    m_status = new QLabel(content);
    m_status->setObjectName("StatusLabel");
    m_status->setVisible(false);
    root->addWidget(m_status);

    m_activityChart = new ChartCard(QStringLiteral("平台活跃趋势"), 280, content);
    m_activityChart->setLegendStyle(ChartLegendStyle::Bottom);
    m_activityChart->setSubtitle(QStringLiteral("每日浏览量与检索次数"));
    root->addWidget(m_activityChart);

    m_contentChart = new ChartCard(QStringLiteral("内容生产趋势"), 280, content);
    m_contentChart->setLegendStyle(ChartLegendStyle::Bottom);
    m_contentChart->setSubtitle(QStringLiteral("每日新增知识与上线知识"));
    root->addWidget(m_contentChart);

    auto *distRow = new QHBoxLayout();
    distRow->setSpacing(16);
    m_statusPie = new ChartCard(QStringLiteral("知识状态分布"), 240, content);
    m_statusPie->setLegendStyle(ChartLegendStyle::LeftVertical);
    m_typePie = new ChartCard(QStringLiteral("知识类型分布"), 240, content);
    m_typePie->setLegendStyle(ChartLegendStyle::LeftVertical);
    distRow->addWidget(m_statusPie, 1);
    distRow->addWidget(m_typePie, 1);
    root->addLayout(distRow);

    m_categoryChart = new ChartCard(QStringLiteral("分类知识量 TOP 10"), 340, content);
    m_categoryChart->setSubtitle(QStringLiteral("各分类已收录知识数量（含各状态）"));
    root->addWidget(m_categoryChart);

    auto *rankRow = new QHBoxLayout();
    rankRow->setSpacing(16);
    m_hotArticleChart = new ChartCard(QStringLiteral("热门知识 · 浏览量 TOP 10"), 320, content);
    m_hotKeywordChart = new ChartCard(QStringLiteral("热门搜索词 TOP 10"), 320, content);
    m_hotKeywordChart->setSubtitle(QStringLiteral("柱高为检索次数；关注无结果词揭示内容缺口"));
    rankRow->addWidget(m_hotArticleChart, 1);
    rankRow->addWidget(m_hotKeywordChart, 1);
    root->addLayout(rankRow);

    auto *opsRow = new QHBoxLayout();
    opsRow->setSpacing(16);
    m_auditChart = new ChartCard(QStringLiteral("审核运营"), 240, content);
    m_auditChart->setLegendStyle(ChartLegendStyle::LeftVertical);
    m_qaChart = new ChartCard(QStringLiteral("智能问答运营"), 240, content);
    m_qaChart->setLegendStyle(ChartLegendStyle::Bottom);
    opsRow->addWidget(m_auditChart, 1);
    opsRow->addWidget(m_qaChart, 1);
    root->addLayout(opsRow);

    root->addStretch();
    scroll->setWidget(content);
    outer->addWidget(scroll);
}

void StatisticsPage::refreshPage() {
    refresh();
}

void StatisticsPage::updatePeriodControls() {
    const bool monthly = isMonthlyTrend();
    if (m_yearLabel) {
        m_yearLabel->setVisible(monthly);
    }
    if (m_yearWindow) {
        m_yearWindow->setVisible(monthly);
    }
    if (m_activityChart) {
        m_activityChart->setSubtitle(monthly ? QStringLiteral("每月浏览量与检索次数")
                                           : QStringLiteral("每日浏览量与检索次数"));
    }
    if (m_contentChart) {
        m_contentChart->setSubtitle(monthly ? QStringLiteral("每月新增知识与上线知识")
                                            : QStringLiteral("每日新增知识与上线知识"));
    }
}

int StatisticsPage::trendDays() const {
    if (!m_periodWindow) {
        return 30;
    }
    const int v = m_periodWindow->currentData().toInt();
    return v > 0 ? v : 90;
}

int StatisticsPage::trendYear() const {
    return m_yearWindow ? m_yearWindow->currentData().toInt() : QDate::currentDate().year();
}

bool StatisticsPage::isMonthlyTrend() const {
    return m_periodWindow && m_periodWindow->currentData().toInt() < 0;
}

void StatisticsPage::refresh() {
    setStatus(QStringLiteral("加载中..."));
    loadOverview();
    loadTrend();
    loadCategoryRank();
    loadHotArticles();
    loadHotKeywords();
    loadAuditOverview();
    loadQaOverview();
}

void StatisticsPage::loadOverview() {
    ApiClient::instance().get("/statistics/overview", [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        const QJsonObject o = r.object();

        m_kpi.articleTotal->setText(QString::number(asLong(o.value("articleTotal"))));
        m_kpi.articleOnline->setText(QString::number(asLong(o.value("articleOnline"))));
        m_kpi.articlePending->setText(QString::number(asLong(o.value("articlePendingAudit"))));
        m_kpi.articleDraft->setText(QString::number(asLong(o.value("articleDraft"))));
        m_kpi.articleOffline->setText(QString::number(asLong(o.value("articleOffline"))));
        m_kpi.viewTotal->setText(QString::number(asLong(o.value("viewTotal"))));
        m_kpi.favoriteTotal->setText(QString::number(asLong(o.value("favoriteTotal"))));
        m_kpi.likeTotal->setText(QString::number(asLong(o.value("likeTotal"))));
        m_kpi.commentTotal->setText(QString::number(asLong(o.value("commentTotal"))));
        m_kpi.categoryTag->setText(QStringLiteral("%1 / %2")
                                       .arg(asLong(o.value("categoryCount")))
                                       .arg(asLong(o.value("tagCount"))));
        m_kpi.today->setText(
            QStringLiteral("<span style='color:#757575'>今日</span> "
                           "<span style='color:#4A4A4A'>浏览 <b>%1</b></span>"
                           "<span style='color:#C8C4BC'> · </span>"
                           "<span style='color:#4A4A4A'>检索 <b>%2</b></span>"
                           "<span style='color:#C8C4BC'> · </span>"
                           "<span style='color:#4A4A4A'>新增知识 <b>%3</b></span>")
                .arg(asLong(o.value("todayViews")))
                .arg(asLong(o.value("todaySearches")))
                .arg(asLong(o.value("todayNewArticles"))));

        QVector<QPair<QString, qint64>> statusSlices;
        for (const QJsonValue &v : o.value("statusDist").toArray()) {
            const QJsonObject item = v.toObject();
            statusSlices.append({ChartTheme::statusLabel(item.value("name").toString()),
                                 asLong(item.value("count"))});
        }
        m_statusPie->setLegendHtml(ChartTheme::compactLegendHtml(statusSlices));
        m_statusPie->setChart(ChartTheme::buildPieChart(statusSlices));

        QVector<QPair<QString, qint64>> typeSlices;
        for (const QJsonValue &v : o.value("typeDist").toArray()) {
            const QJsonObject item = v.toObject();
            typeSlices.append({ChartTheme::typeLabel(item.value("name").toString()),
                               asLong(item.value("count"))});
        }
        m_typePie->setLegendHtml(ChartTheme::compactLegendHtml(typeSlices));
        m_typePie->setChart(ChartTheme::buildPieChart(typeSlices));

        setStatus(QString());
    });
}

void StatisticsPage::loadTrend() {
    const QString path = isMonthlyTrend()
                             ? QStringLiteral("/statistics/trend?year=%1").arg(trendYear())
                             : QStringLiteral("/statistics/trend?days=%1").arg(trendDays());
    ApiClient::instance().get(path, [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        const QJsonObject d = r.object();
        const bool monthly = d.value("granularity").toString() == QStringLiteral("month");
        const QStringList cats = datesFromTrend(d.value("views").toArray(), monthly);
        const QVector<qint64> views = countsFromTrend(d.value("views").toArray());
        const QVector<qint64> searches = countsFromTrend(d.value("searches").toArray());
        const QVector<qint64> newArts = countsFromTrend(d.value("newArticles").toArray());
        const QVector<qint64> onlineArts = countsFromTrend(d.value("onlineArticles").toArray());

        m_activityChart->setLegendHtml(
            ChartTheme::compactSeriesLegendHtml({QStringLiteral("浏览"), QStringLiteral("检索")}));
        m_activityChart->setChart(ChartTheme::buildLineChart(
            cats, {{QStringLiteral("浏览"), views}, {QStringLiteral("检索"), searches}}));

        m_contentChart->setLegendHtml(
            ChartTheme::compactSeriesLegendHtml({QStringLiteral("新增"), QStringLiteral("上线")}));
        m_contentChart->setChart(ChartTheme::buildLineChart(
            cats, {{QStringLiteral("新增"), newArts}, {QStringLiteral("上线"), onlineArts}}));
    });
}

void StatisticsPage::loadCategoryRank() {
    ApiClient::instance().get("/statistics/category-rank?limit=10", [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        QStringList labels;
        QVector<qint64> values;
        for (const QJsonValue &v : r.data.toArray()) {
            const QJsonObject o = v.toObject();
            labels.prepend(o.value("categoryName").toString().trimmed());
            values.prepend(asLong(o.value("articleCount")));
        }
        m_categoryChart->setChart(
            ChartTheme::buildHorizontalBarChart(labels, values, QColor(), 120));
        m_categoryChart->setChartMinHeight(qMax(320, labels.size() * 32 + 48));
    });
}

void StatisticsPage::loadHotArticles() {
    ApiClient::instance().get("/statistics/hot-article?limit=10", [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        QStringList labels;
        QStringList fullLabels;
        QVector<qint64> values;
        for (const QJsonValue &v : r.data.toArray()) {
            const QJsonObject o = v.toObject();
            const QString title = o.value("title").toString().trimmed();
            fullLabels.prepend(title);
            labels.prepend(truncateLabel(title, 10));
            values.prepend(asLong(o.value("viewCount")));
        }
        m_hotArticleChart->setChart(ChartTheme::buildHorizontalBarChart(
            labels, values, QColor("#6B7F74"), 72, fullLabels));
        m_hotArticleChart->setChartMinHeight(qMax(300, labels.size() * 32 + 48));
    });
}

void StatisticsPage::loadHotKeywords() {
    const QString path =
        QStringLiteral("/statistics/hot-keyword?limit=10&days=%1").arg(trendDays());
    ApiClient::instance().get(path, [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        QStringList labels;
        QStringList fullLabels;
        QVector<qint64> values;
        for (const QJsonValue &v : r.data.toArray()) {
            const QJsonObject o = v.toObject();
            const QString kw = o.value("keyword").toString().trimmed();
            fullLabels.prepend(kw);
            labels.prepend(truncateLabel(kw, 10));
            values.prepend(asLong(o.value("searchCount")));
        }
        m_hotKeywordChart->setChart(ChartTheme::buildHorizontalBarChart(
            labels, values, QColor("#8A9690"), 72, fullLabels));
        m_hotKeywordChart->setChartMinHeight(qMax(300, labels.size() * 32 + 48));
    });
}

void StatisticsPage::loadAuditOverview() {
    ApiClient::instance().get("/statistics/audit-overview", [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        const QJsonObject o = r.object();
        const qint64 pending = asLong(o.value("pendingAudit"));
        const qint64 pass = asLong(o.value("passCount"));
        const qint64 reject = asLong(o.value("rejectCount"));
        const double rate = o.value("passRate").toDouble(-1);

        QString sub = QStringLiteral("当前待审 %1 篇").arg(pending);
        if (rate >= 0) {
            sub += QStringLiteral(" · 历史通过率 %1%").arg(QString::number(rate * 100, 'f', 1));
        }
        m_auditChart->setSubtitle(sub);

        QVector<QPair<QString, qint64>> slices;
        for (const QJsonValue &v : o.value("resultDist").toArray()) {
            const QJsonObject item = v.toObject();
            slices.append({ChartTheme::statusLabel(item.value("name").toString()),
                           asLong(item.value("count"))});
        }
        if (pending > 0) {
            slices.prepend({QStringLiteral("待审核"), pending});
        }
        m_auditChart->setLegendHtml(ChartTheme::compactLegendHtml(slices));
        m_auditChart->setChart(ChartTheme::buildPieChart(slices));
    });
}

void StatisticsPage::loadQaOverview() {
    const QString path = QStringLiteral("/statistics/qa-overview?days=%1").arg(trendDays());
    ApiClient::instance().get(path, [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        const QJsonObject o = r.object();
        const qint64 sessions = asLong(o.value("sessionTotal"));
        const qint64 messages = asLong(o.value("messageTotal"));
        const qint64 helpful = asLong(o.value("helpfulCount"));
        const qint64 unhelpful = asLong(o.value("unhelpfulCount"));
        const double rate = o.value("helpfulRate").toDouble(-1);

        QString sub = QStringLiteral("会话 %1 · 消息 %2 · 反馈 👍%3 / 👎%4")
                          .arg(sessions)
                          .arg(messages)
                          .arg(helpful)
                          .arg(unhelpful);
        if (rate >= 0) {
            sub += QStringLiteral(" · 有用率 %1%").arg(QString::number(rate * 100, 'f', 1));
        }
        m_qaChart->setSubtitle(sub);

        const QStringList cats = datesFromTrend(o.value("sessionTrend").toArray(), false);
        const QVector<qint64> trend = countsFromTrend(o.value("sessionTrend").toArray());
        m_qaChart->setLegendHtml(ChartTheme::compactSeriesLegendHtml({QStringLiteral("会话")}));
        m_qaChart->setChart(ChartTheme::buildLineChart(cats, {{QStringLiteral("会话"), trend}}));
        if (!cats.isEmpty()) {
            m_qaChart->setChartMinHeight(260);
        }
    });
}

void StatisticsPage::setStatus(const QString &text, bool error) {
    m_status->setVisible(!text.isEmpty());
    m_status->setText(text);
    m_status->setStyleSheet(error ? "color:#B94A48;" : "color:#757575;");
    if (error && !text.isEmpty()) {
        notify::warn(this, text);
    }
}

} // namespace kb
