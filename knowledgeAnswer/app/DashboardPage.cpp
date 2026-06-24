#include "app/DashboardPage.h"

#include "app/ArticleDetailDialog.h"
#include "app/RefreshablePage.h"
#include "common/TableStyle.h"
#include "core/auth/Session.h"
#include "core/config/PinnedTagsStore.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QAbstractItemView>
#include <QDate>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QScrollArea>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>

namespace kb {

namespace {

qint64 asLong(const QJsonValue &v) {
    return static_cast<qint64>(v.toDouble());
}

// 指标卡：白底圆角，大号数值在上、灰色说明在下。数值标签经 valueLabel 带出供刷新更新。
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

QString roleLabel(const QString &code) {
    if (code == QStringLiteral("ADMIN")) return QStringLiteral("管理员");
    if (code == QStringLiteral("KNOWLEDGE_ADMIN")) return QStringLiteral("知识管理员");
    if (code == QStringLiteral("AUDITOR")) return QStringLiteral("审核员");
    if (code == QStringLiteral("AGENT")) return QStringLiteral("坐席");
    if (code == QStringLiteral("USER")) return QStringLiteral("普通用户");
    return code;
}

// 深度优先：用户菜单树里是否存在某路由名（服务端已按权限过滤菜单，存在即该角色可见可点）。
bool routeExists(const QVector<MenuItem> &items, const QString &name) {
    for (const MenuItem &m : items) {
        if (m.name == name) {
            return true;
        }
        if (routeExists(m.children, name)) {
            return true;
        }
    }
    return false;
}

// 数据量少的榜单：固定展示全部行，纵向由页面滚动区承载。
void fitTableToAllRows(QTableWidget *table) {
    if (!table) {
        return;
    }
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    const int rowHeight = table->verticalHeader()->defaultSectionSize();
    const int bodyHeight = table->rowCount() > 0 ? table->rowCount() * rowHeight : rowHeight;
    const int headerHeight =
        qMax(table->horizontalHeader()->height(), rowHeight);
    table->setFixedHeight(headerHeight + bodyHeight + table->frameWidth() * 2);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

} // namespace

DashboardPage::DashboardPage(const QString &title, QWidget *parent)
    : QWidget(parent), m_title(title) {
    m_canStats = Session::instance().hasPermission(QStringLiteral("statistics:view"));
    m_canHotArticles = Session::instance().hasPermission(QStringLiteral("knowledge:search"));
    buildUi();
    if (m_canStats) {
        loadOverview();
    }
    if (m_canHotArticles) {
        loadRecommendations();
        loadHotArticles();
    }
}

void DashboardPage::buildUi() {
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    auto *scroll = new QScrollArea(this);
    scroll->setObjectName("PageScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *content = new QWidget(scroll);
    auto *root = new QVBoxLayout(content);
    root->setContentsMargins(28, 20, 28, 20);
    root->setSpacing(14);

    // —— 欢迎区（所有角色）——
    const CurrentUser &u = Session::instance().user();
    const QString who = u.realName.isEmpty() ? u.username : u.realName;
    const QVector<QString> &roles = Session::instance().roles();
    const QString role = roles.isEmpty() ? QString() : roleLabel(roles.first());
    const QString today =
        QLocale(QLocale::Chinese).toString(QDate::currentDate(), QStringLiteral("yyyy年M月d日 dddd"));
    QStringList subParts;
    if (!role.isEmpty()) {
        subParts << role;
    }
    subParts << today << QStringLiteral("欢迎回到智能客服知识库");

    auto *banner = new QFrame(content);
    banner->setObjectName("WelcomeBanner");
    auto *bannerLayout = new QVBoxLayout(banner);
    bannerLayout->setContentsMargins(24, 20, 24, 20);
    bannerLayout->setSpacing(6);
    auto *welcomeTitle = new QLabel(
        QStringLiteral("你好，%1").arg(who.isEmpty() ? QStringLiteral("欢迎") : who), banner);
    welcomeTitle->setObjectName("WelcomeTitle");
    auto *welcomeHint = new QLabel(subParts.join(QStringLiteral(" · ")), banner);
    welcomeHint->setObjectName("WelcomeHint");
    bannerLayout->addWidget(welcomeTitle);
    bannerLayout->addWidget(welcomeHint);
    root->addWidget(banner);

    // —— 统计区（仅 statistics:view）——
    if (m_canStats) {
        m_status = new QLabel(content);
        m_status->setObjectName("StatusLabel");
        root->addWidget(m_status);

        auto *grid = new QGridLayout();
        grid->setSpacing(12);
        grid->addWidget(makeCard(QStringLiteral("知识总数"), m_cardTotal, content), 0, 0);
        grid->addWidget(makeCard(QStringLiteral("已上线"), m_cardOnline, content), 0, 1);
        grid->addWidget(makeCard(QStringLiteral("待审核"), m_cardPending, content), 0, 2);
        grid->addWidget(makeCard(QStringLiteral("草稿"), m_cardDraft, content), 0, 3);
        for (int c = 0; c < 4; ++c) {
            grid->setColumnStretch(c, 1);
        }
        root->addLayout(grid);

        m_todayLine = new QLabel(content);
        m_todayLine->setObjectName("MutedLine");
        root->addWidget(m_todayLine);
    }

    if (m_canHotArticles) {
        if (!m_status) {
            m_status = new QLabel(content);
            m_status->setObjectName("StatusLabel");
            root->addWidget(m_status);
        }

        auto *recommendTitle = new QLabel(QStringLiteral("推荐知识"), content);
        recommendTitle->setObjectName("SectionTitle");
        root->addWidget(recommendTitle);

        m_recommendHint = new QLabel(content);
        m_recommendHint->setObjectName("MutedLabel");
        m_recommendHint->setWordWrap(true);
        root->addWidget(m_recommendHint);

        m_recommendArticles = new QTableWidget(content);
        m_recommendArticles->setColumnCount(4);
        m_recommendArticles->setHorizontalHeaderLabels({QStringLiteral("#"), QStringLiteral("标题"),
                                                        QStringLiteral("分类"),
                                                        QStringLiteral("兴趣/热门")});
        TableStyle::configureRankedTable(m_recommendArticles, 1);
        connect(m_recommendArticles, &QTableWidget::doubleClicked, this,
                [this]() { openArticleDetail(m_recommendArticles); });
        root->addWidget(m_recommendArticles);

        auto *hotTitle = new QLabel(QStringLiteral("热门知识 · 浏览量 TOP 5"), content);
        hotTitle->setObjectName("SectionTitle");
        root->addWidget(hotTitle);

        m_hotArticles = new QTableWidget(content);
        m_hotArticles->setColumnCount(4);
        m_hotArticles->setHorizontalHeaderLabels({QStringLiteral("#"), QStringLiteral("标题"),
                                                  QStringLiteral("分类"), QStringLiteral("浏览")});
        TableStyle::configureRankedTable(m_hotArticles, 1);
        connect(m_hotArticles, &QTableWidget::doubleClicked, this,
                [this]() { openArticleDetail(m_hotArticles); });
        root->addWidget(m_hotArticles);
    }

    // —— 快捷入口（所有角色，按菜单可见性显隐）——
    struct QuickEntry {
        QString label;
        QString route;
    };
    const QVector<QuickEntry> entries = {
        {QStringLiteral("知识检索"), QStringLiteral("knowledge.search")},
        {QStringLiteral("我的收藏"), QStringLiteral("favorite")},
        {QStringLiteral("我的分享"), QStringLiteral("share")},
    };
    const QVector<MenuItem> &menus = Session::instance().menus();
    QVector<QuickEntry> available;
    for (const QuickEntry &e : entries) {
        if (routeExists(menus, e.route)) {
            available << e;
        }
    }
    if (!available.isEmpty()) {
        auto *quickTitle = new QLabel(QStringLiteral("快捷入口"), content);
        quickTitle->setObjectName("SectionTitle");
        root->addWidget(quickTitle);

        auto *quickRow = new QHBoxLayout();
        quickRow->setSpacing(12);
        for (const QuickEntry &e : available) {
            auto *btn = new QPushButton(e.label, content);
            btn->setObjectName("GhostButton");
            btn->setMinimumHeight(44);
            const QString route = e.route;
            connect(btn, &QPushButton::clicked, this, [this, route]() { emit navigateRequested(route); });
            quickRow->addWidget(btn);
        }
        quickRow->addStretch();
        root->addLayout(quickRow);
    }

    root->addStretch();

    scroll->setWidget(content);
    pageLayout->addWidget(scroll);
}

void DashboardPage::refreshPage() {
    if (m_canStats) {
        loadOverview();
    }
    if (m_canHotArticles) {
        loadRecommendations();
        loadHotArticles();
    }
}

void DashboardPage::loadOverview() {
    setStatus(QStringLiteral("加载中..."));
    ApiClient::instance().get("/statistics/overview", [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        const QJsonObject o = r.object();
        if (m_cardTotal) {
            m_cardTotal->setText(QString::number(asLong(o.value("articleTotal"))));
        }
        if (m_cardOnline) {
            m_cardOnline->setText(QString::number(asLong(o.value("articleOnline"))));
        }
        if (m_cardPending) {
            m_cardPending->setText(QString::number(asLong(o.value("articlePendingAudit"))));
        }
        if (m_cardDraft) {
            m_cardDraft->setText(QString::number(asLong(o.value("articleDraft"))));
        }
        if (m_todayLine) {
            m_todayLine->setText(QStringLiteral("今日浏览 %1 · 今日检索 %2 · 今日新增知识 %3")
                                     .arg(asLong(o.value("todayViews")))
                                     .arg(asLong(o.value("todaySearches")))
                                     .arg(asLong(o.value("todayNewArticles"))));
        }
        setStatus(QString());
    });
}

void DashboardPage::loadRecommendations() {
    const QString url =
        QStringLiteral("/knowledge/recommend/home?limit=5&pinnedTagIds=%1").arg(pinnedTagIdsParam());
    ApiClient::instance().get(url, [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        if (!m_recommendArticles) {
            return;
        }
        const QJsonObject data = r.object();
        const QJsonArray list = data.value("items").toArray();
        const QJsonObject summary = data.value("profileSummary").toObject();
        const bool fallback = data.value("fallback").toBool();

        QStringList hintParts;
        const QJsonArray topTags = summary.value("topTags").toArray();
        for (const QJsonValue &v : topTags) {
            const QString name = v.toObject().value("name").toString();
            if (!name.isEmpty()) {
                hintParts << name;
            }
        }
        if (hintParts.isEmpty()) {
            const QJsonArray keywords = summary.value("keywords").toArray();
            for (const QJsonValue &v : keywords) {
                const QString kw = v.toString();
                if (!kw.isEmpty()) {
                    hintParts << kw;
                }
            }
        }
        if (m_recommendHint) {
            if (hintParts.isEmpty()) {
                m_recommendHint->setText(fallback ? QStringLiteral("根据全站热门为你推荐（未识别到近期搜索/标签行为）")
                                                    : QStringLiteral("根据你的兴趣为你推荐"));
            } else {
                QString hint = QStringLiteral("猜你感兴趣：%1").arg(hintParts.join(QStringLiteral(" · ")));
                if (fallback) {
                    hint += QStringLiteral("（已降级为热门推荐）");
                }
                m_recommendHint->setText(hint);
            }
        }

        m_recommendArticles->setRowCount(0);
        int rank = 0;
        for (const QJsonValue &v : list) {
            const QJsonObject o = v.toObject();
            const int row = m_recommendArticles->rowCount();
            m_recommendArticles->insertRow(row);
            auto *rankItem = new QTableWidgetItem(QString::number(++rank));
            rankItem->setData(Qt::UserRole, o.value("id").toVariant());
            rankItem->setTextAlignment(Qt::AlignCenter);
            m_recommendArticles->setItem(row, 0, rankItem);
            m_recommendArticles->setItem(row, 1, new QTableWidgetItem(o.value("title").toString()));
            m_recommendArticles->setItem(row, 2, new QTableWidgetItem(o.value("categoryName").toString()));
            const double hot = o.value("hotScore").toDouble();
            const double match = o.value("matchScore").toDouble();
            m_recommendArticles->setItem(
                row, 3, new QTableWidgetItem(QStringLiteral("%1 / %2")
                                                   .arg(QString::number(match, 'f', 2))
                                                   .arg(QString::number(hot, 'f', 2))));
        }
        TableStyle::setItemTooltipFromText(m_recommendArticles);
        fitTableToAllRows(m_recommendArticles);
    });
}

QString DashboardPage::pinnedTagIdsParam() {
    const qint64 userId = Session::instance().user().id;
    const QVector<qint64> ids = PinnedTagsStore::instance().pinnedTagIds(userId);
    QStringList parts;
    for (qint64 id : ids) {
        parts << QString::number(id);
    }
    return parts.join(QStringLiteral(","));
}

void DashboardPage::loadHotArticles() {
    ApiClient::instance().get("/statistics/hot-article?limit=5", [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        if (!m_hotArticles) {
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
            m_hotArticles->setItem(row, 3,
                                   new QTableWidgetItem(QString::number(asLong(o.value("viewCount")))));
        }
        TableStyle::setItemTooltipFromText(m_hotArticles);
        fitTableToAllRows(m_hotArticles);
    });
}

void DashboardPage::openArticleDetail(QTableWidget *table) {
    if (!table) {
        return;
    }
    const int row = table->currentRow();
    if (row < 0 || !table->item(row, 0)) {
        return;
    }
    const qint64 id = table->item(row, 0)->data(Qt::UserRole).toLongLong();
    if (id <= 0) {
        return;
    }
    if (!Session::instance().hasPermission(QStringLiteral("knowledge:search"))) {
        notify::warn(this, QStringLiteral("当前角色无知识查看权限"));
        return;
    }
    ArticleDetailDialog dlg(id, this);
    dlg.exec();
}

void DashboardPage::setStatus(const QString &text, bool error) {
    if (!m_status) {
        return;
    }
    m_status->setText(text);
    m_status->setStyleSheet(error ? "color:#B94A48;" : "color:#757575;");
    if (error && !text.isEmpty()) {
        notify::warn(this, text);
    }
}

} // namespace kb
