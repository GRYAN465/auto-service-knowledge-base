#include "app/StatisticsPage.h"

#include "app/ArticleDetailDialog.h"
#include "app/RefreshablePage.h"
#include "common/TableStyle.h"
#include "core/auth/Session.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace kb {

namespace {

QString typeLabel(const QString &code) {
    if (code == "SCRIPT") return QStringLiteral("话术");
    if (code == "TRAIN") return QStringLiteral("培训");
    if (code == "PRODUCT") return QStringLiteral("产品");
    if (code == "OFFICE") return QStringLiteral("办公");
    return code.isEmpty() ? QStringLiteral("未分类") : code;
}

qint64 asLong(const QJsonValue &v) {
    return static_cast<qint64>(v.toDouble());
}

// 构建一张指标卡：白底圆角，大号数值在上、灰色说明在下。数值标签由 valueLabel 带出供刷新更新。
QFrame *makeCard(const QString &caption, QLabel *&valueLabel, QWidget *parent) {
    auto *card = new QFrame(parent);
    card->setObjectName("StatCard");
    auto *lay = new QVBoxLayout(card);
    lay->setContentsMargins(16, 14, 16, 14);
    lay->setSpacing(6);
    valueLabel = new QLabel(QStringLiteral("—"), card);
    valueLabel->setObjectName("StatCardValue");
    auto *cap = new QLabel(caption, card);
    cap->setObjectName("StatCardLabel");
    lay->addWidget(valueLabel);
    lay->addWidget(cap);
    return card;
}

void setupTable(QTableWidget *table, const QStringList &headers) {
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    TableStyle::applyDataTable(table);
}

} // namespace

StatisticsPage::StatisticsPage(const QString &title, QWidget *parent)
    : QWidget(parent), m_title(title) {
    buildUi();
    refresh();
}

void StatisticsPage::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 24);
    root->setSpacing(14);

    m_status = new QLabel(this);
    m_status->setObjectName("StatusLabel");
    root->addWidget(m_status);

    // 指标卡：2 行 × 4 列
    auto *grid = new QGridLayout();
    grid->setSpacing(12);
    grid->addWidget(makeCard(QStringLiteral("知识总数"), m_cardArticleTotal, this), 0, 0);
    grid->addWidget(makeCard(QStringLiteral("已上线"), m_cardOnline, this), 0, 1);
    grid->addWidget(makeCard(QStringLiteral("待审核"), m_cardPending, this), 0, 2);
    grid->addWidget(makeCard(QStringLiteral("草稿"), m_cardDraft, this), 0, 3);
    grid->addWidget(makeCard(QStringLiteral("累计浏览"), m_cardViews, this), 1, 0);
    grid->addWidget(makeCard(QStringLiteral("累计收藏"), m_cardFavorites, this), 1, 1);
    grid->addWidget(makeCard(QStringLiteral("累计点赞"), m_cardLikes, this), 1, 2);
    grid->addWidget(makeCard(QStringLiteral("累计评论"), m_cardComments, this), 1, 3);
    for (int c = 0; c < 4; ++c) {
        grid->setColumnStretch(c, 1);
    }
    root->addLayout(grid);

    // 说明行：今日动态 / 规模与类型分布
    m_todayLine = new QLabel(this);
    m_todayLine->setObjectName("MutedLine");
    m_scaleLine = new QLabel(this);
    m_scaleLine->setObjectName("MutedLine");
    m_scaleLine->setWordWrap(true);
    root->addWidget(m_todayLine);
    root->addWidget(m_scaleLine);

    // 两块榜单并排：热门知识（宽）/ 热门搜索词（窄）
    auto *panels = new QHBoxLayout();
    panels->setSpacing(16);

    auto *leftBox = new QVBoxLayout();
    leftBox->setSpacing(8);
    auto *hotTitle = new QLabel(QStringLiteral("热门知识 · 浏览量 TOP 10"), this);
    hotTitle->setObjectName("SectionTitle");
    m_hotArticles = new QTableWidget(this);
    setupTable(m_hotArticles, {QStringLiteral("#"), QStringLiteral("标题"), QStringLiteral("分类"),
                               QStringLiteral("类型"), QStringLiteral("浏览"), QStringLiteral("收藏"),
                               QStringLiteral("评论")});
    TableStyle::configureRankedTable(m_hotArticles, 1);
    connect(m_hotArticles, &QTableWidget::doubleClicked, this, &StatisticsPage::openArticleDetail);
    leftBox->addWidget(hotTitle);
    leftBox->addWidget(m_hotArticles, 1);

    auto *rightBox = new QVBoxLayout();
    rightBox->setSpacing(8);
    auto *kwHeader = new QHBoxLayout();
    auto *kwTitle = new QLabel(QStringLiteral("热门搜索词"), this);
    kwTitle->setObjectName("SectionTitle");
    m_keywordWindow = new QComboBox(this);
    m_keywordWindow->addItem(QStringLiteral("近 7 天"), 7);
    m_keywordWindow->addItem(QStringLiteral("近 30 天"), 30);
    m_keywordWindow->addItem(QStringLiteral("近 90 天"), 90);
    m_keywordWindow->addItem(QStringLiteral("全部"), 0);
    m_keywordWindow->setCurrentIndex(1); // 默认近 30 天
    connect(m_keywordWindow, &QComboBox::currentIndexChanged, this, [this]() { loadHotKeywords(); });
    kwHeader->addWidget(kwTitle);
    kwHeader->addStretch();
    kwHeader->addWidget(m_keywordWindow);
    m_hotKeywords = new QTableWidget(this);
    setupTable(m_hotKeywords, {QStringLiteral("#"), QStringLiteral("搜索词"),
                               QStringLiteral("次数"), QStringLiteral("无结果")});
    TableStyle::configureRankedTable(m_hotKeywords, 1);
    rightBox->addLayout(kwHeader);
    rightBox->addWidget(m_hotKeywords, 1);

    panels->addLayout(leftBox, 3);
    panels->addLayout(rightBox, 2);
    root->addLayout(panels, 1);
}

void StatisticsPage::refreshPage() {
    refresh();
}

void StatisticsPage::refresh() {
    setStatus(QStringLiteral("加载中..."));
    loadOverview();
    loadHotArticles();
    loadHotKeywords();
}

void StatisticsPage::loadOverview() {
    ApiClient::instance().get("/statistics/overview", [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        const QJsonObject o = r.object();
        m_cardArticleTotal->setText(QString::number(asLong(o.value("articleTotal"))));
        m_cardOnline->setText(QString::number(asLong(o.value("articleOnline"))));
        m_cardPending->setText(QString::number(asLong(o.value("articlePendingAudit"))));
        m_cardDraft->setText(QString::number(asLong(o.value("articleDraft"))));
        m_cardViews->setText(QString::number(asLong(o.value("viewTotal"))));
        m_cardFavorites->setText(QString::number(asLong(o.value("favoriteTotal"))));
        m_cardLikes->setText(QString::number(asLong(o.value("likeTotal"))));
        m_cardComments->setText(QString::number(asLong(o.value("commentTotal"))));

        m_todayLine->setText(QStringLiteral("今日浏览 %1 · 今日检索 %2 · 今日新增知识 %3")
                                 .arg(asLong(o.value("todayViews")))
                                 .arg(asLong(o.value("todaySearches")))
                                 .arg(asLong(o.value("todayNewArticles"))));

        // 类型分布拼接为一行
        QStringList typeParts;
        for (const QJsonValue &v : o.value("typeDist").toArray()) {
            const QJsonObject t = v.toObject();
            typeParts << QStringLiteral("%1 %2").arg(typeLabel(t.value("name").toString()))
                             .arg(asLong(t.value("count")));
        }
        QString scale = QStringLiteral("已下线 %1 · 分类 %2 · 标签 %3")
                            .arg(asLong(o.value("articleOffline")))
                            .arg(asLong(o.value("categoryCount")))
                            .arg(asLong(o.value("tagCount")));
        if (!typeParts.isEmpty()) {
            scale += QStringLiteral("   |   知识类型：") + typeParts.join(QStringLiteral(" · "));
        }
        m_scaleLine->setText(scale);

        setStatus(QString());
    });
}

void StatisticsPage::loadHotArticles() {
    ApiClient::instance().get("/statistics/hot-article?limit=10", [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        const QJsonArray list = r.data.toArray();
        m_hotArticles->setRowCount(0);
        int rank = 0;
        for (const QJsonValue &v : list) {
            const QJsonObject o = v.toObject();
            const int row = m_hotArticles->rowCount();
            m_hotArticles->insertRow(row);
            auto *rankItem = new QTableWidgetItem(QString::number(++rank));
            rankItem->setData(Qt::UserRole, o.value("id").toVariant());
            rankItem->setTextAlignment(Qt::AlignCenter);
            m_hotArticles->setItem(row, 0, rankItem);
            m_hotArticles->setItem(row, 1, new QTableWidgetItem(o.value("title").toString()));
            m_hotArticles->setItem(row, 2, new QTableWidgetItem(o.value("categoryName").toString()));
            m_hotArticles->setItem(row, 3, new QTableWidgetItem(typeLabel(o.value("knowledgeType").toString())));
            m_hotArticles->setItem(row, 4, new QTableWidgetItem(QString::number(asLong(o.value("viewCount")))));
            m_hotArticles->setItem(row, 5, new QTableWidgetItem(QString::number(asLong(o.value("favoriteCount")))));
            m_hotArticles->setItem(row, 6, new QTableWidgetItem(QString::number(asLong(o.value("commentCount")))));
        }
        TableStyle::setItemTooltipFromText(m_hotArticles);
    });
}

void StatisticsPage::loadHotKeywords() {
    const int days = m_keywordWindow ? m_keywordWindow->currentData().toInt() : 30;
    const QString path = QStringLiteral("/statistics/hot-keyword?limit=10&days=%1").arg(days);
    ApiClient::instance().get(path, [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        const QJsonArray list = r.data.toArray();
        m_hotKeywords->setRowCount(0);
        int rank = 0;
        for (const QJsonValue &v : list) {
            const QJsonObject o = v.toObject();
            const int row = m_hotKeywords->rowCount();
            m_hotKeywords->insertRow(row);
            auto *rankItem = new QTableWidgetItem(QString::number(++rank));
            rankItem->setTextAlignment(Qt::AlignCenter);
            m_hotKeywords->setItem(row, 0, rankItem);
            m_hotKeywords->setItem(row, 1, new QTableWidgetItem(o.value("keyword").toString()));
            m_hotKeywords->setItem(row, 2, new QTableWidgetItem(QString::number(asLong(o.value("searchCount")))));
            m_hotKeywords->setItem(row, 3, new QTableWidgetItem(QString::number(asLong(o.value("zeroCount")))));
        }
        TableStyle::setItemTooltipFromText(m_hotKeywords);
    });
}

void StatisticsPage::openArticleDetail() {
    const int row = m_hotArticles->currentRow();
    if (row < 0 || !m_hotArticles->item(row, 0)) {
        return;
    }
    const qint64 id = m_hotArticles->item(row, 0)->data(Qt::UserRole).toLongLong();
    if (id <= 0) {
        return;
    }
    if (!Session::instance().hasPermission("knowledge:search")) {
        notify::warn(this, QStringLiteral("当前角色无知识查看权限"));
        return;
    }
    ArticleDetailDialog dlg(id, this);
    dlg.exec();
}

void StatisticsPage::setStatus(const QString &text, bool error) {
    m_status->setText(text);
    m_status->setStyleSheet(error ? "color:#B94A48;" : "color:#757575;");
    if (error && !text.isEmpty()) {
        notify::warn(this, text);
    }
}

} // namespace kb
