#include "app/DashboardPage.h"

#include "app/ArticleDetailDialog.h"
#include "app/ArticleFeedCard.h"
#include "app/RefreshablePage.h"
#include "common/NavIcons.h"
#include "core/auth/Session.h"
#include "core/config/PinnedTagsStore.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QDate>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLocale>
#include <QMouseEvent>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <functional>

namespace kb {

namespace {

qint64 asLong(const QJsonValue &v) {
    return static_cast<qint64>(v.toDouble());
}

QString roleLabel(const QString &code) {
    if (code == QStringLiteral("ADMIN")) {
        return QStringLiteral("管理员");
    }
    if (code == QStringLiteral("KNOWLEDGE_ADMIN")) {
        return QStringLiteral("知识管理员");
    }
    if (code == QStringLiteral("AUDITOR")) {
        return QStringLiteral("审核员");
    }
    if (code == QStringLiteral("AGENT")) {
        return QStringLiteral("坐席");
    }
    if (code == QStringLiteral("USER")) {
        return QStringLiteral("普通用户");
    }
    return code;
}

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

QFrame *makeKpiTile(const QString &caption, QLabel *&valueOut, QLabel *&captionOut, QWidget *parent,
                    bool accent = false) {
    auto *tile = new QFrame(parent);
    tile->setObjectName(QStringLiteral("KpiTile"));
    auto *lay = new QVBoxLayout(tile);
    lay->setContentsMargins(14, 12, 14, 12);
    lay->setSpacing(4);
    valueOut = new QLabel(QStringLiteral("—"), tile);
    valueOut->setObjectName(accent ? QStringLiteral("KpiTileValueAccent") : QStringLiteral("KpiTileValue"));
    captionOut = new QLabel(caption, tile);
    captionOut->setObjectName(QStringLiteral("KpiTileLabel"));
    lay->addWidget(valueOut);
    lay->addWidget(captionOut);
    return tile;
}

QToolButton *makeQuickEntry(const QString &label, const QString &route, QWidget *parent) {
    auto *btn = new QToolButton(parent);
    btn->setObjectName(QStringLiteral("QuickEntryTile"));
    btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setText(label);
    btn->setFixedSize(76, 72);
    const QString iconPath = NavIcons::pathForRoute(route);
    if (!iconPath.isEmpty()) {
        btn->setIcon(QIcon(iconPath));
        btn->setIconSize(QSize(24, 24));
    }
    return btn;
}

class HotRankRow : public QFrame {
public:
    explicit HotRankRow(int rank, const QJsonObject &article, QWidget *parent = nullptr)
        : QFrame(parent), m_articleId(asLong(article.value("id"))) {
        setObjectName(QStringLiteral("HotRankRow"));
        setCursor(Qt::PointingHandCursor);
        auto *lay = new QHBoxLayout(this);
        lay->setContentsMargins(12, 10, 12, 10);
        lay->setSpacing(10);

        auto *rankLabel = new QLabel(QString::number(rank), this);
        rankLabel->setObjectName(QStringLiteral("HotRankIndex"));
        rankLabel->setFixedWidth(22);
        rankLabel->setAlignment(Qt::AlignCenter);

        auto *title = new QLabel(article.value("title").toString(), this);
        title->setObjectName(QStringLiteral("HotRankTitle"));

        auto *views = new QLabel(QString::number(asLong(article.value("viewCount"))), this);
        views->setObjectName(QStringLiteral("HotRankViews"));
        views->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        lay->addWidget(rankLabel);
        lay->addWidget(title, 1);
        lay->addWidget(views);
        setToolTip(article.value("title").toString());
    }

    std::function<void(qint64)> onClicked;

protected:
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && m_articleId > 0 && onClicked) {
            onClicked(m_articleId);
        }
        QFrame::mouseReleaseEvent(event);
    }

private:
    qint64 m_articleId = 0;
};

void clearLayout(QLayout *layout) {
    if (!layout) {
        return;
    }
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
}

} // namespace

DashboardPage::DashboardPage(const QString &title, QWidget *parent)
    : QWidget(parent), m_title(title) {
    m_canStats = Session::instance().hasPermission(QStringLiteral("statistics:view"));
    m_canHotArticles = Session::instance().hasPermission(QStringLiteral("knowledge:search"));
    buildUi();
    loadMyKpi();
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
    scroll->setObjectName(QStringLiteral("PageScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *content = new QWidget(scroll);
    auto *root = new QVBoxLayout(content);
    root->setContentsMargins(28, 20, 28, 24);
    root->setSpacing(16);

    const CurrentUser &u = Session::instance().user();
    const QString who = u.realName.isEmpty() ? u.username : u.realName;
    const QVector<QString> &roles = Session::instance().roles();
    const QString role = roles.isEmpty() ? QString() : roleLabel(roles.first());
    const QString today =
        QLocale(QLocale::Chinese).toString(QDate::currentDate(), QStringLiteral("M月d日 dddd"));
    QStringList subParts;
    if (!role.isEmpty()) {
        subParts << role;
    }
    subParts << today;

    auto *welcomeCol = new QVBoxLayout();
    welcomeCol->setSpacing(4);
    auto *welcomeTitle = new QLabel(
        QStringLiteral("你好，%1").arg(who.isEmpty() ? QStringLiteral("欢迎") : who), content);
    welcomeTitle->setObjectName(QStringLiteral("WelcomeTitle"));
    auto *welcomeHint = new QLabel(subParts.join(QStringLiteral(" · ")), content);
    welcomeHint->setObjectName(QStringLiteral("WelcomeHint"));
    welcomeCol->addWidget(welcomeTitle);
    welcomeCol->addWidget(welcomeHint);
    root->addLayout(welcomeCol);

    struct QuickEntry {
        QString label;
        QString route;
    };
    const QVector<QuickEntry> allEntries = {
        {QStringLiteral("知识检索"), QStringLiteral("knowledge.search")},
        {QStringLiteral("智能问答"), QStringLiteral("qa")},
        {QStringLiteral("知识管理"), QStringLiteral("knowledge.manage")},
        {QStringLiteral("审核中心"), QStringLiteral("audit")},
        {QStringLiteral("数据统计"), QStringLiteral("statistics")},
        {QStringLiteral("个人中心"), QStringLiteral("personal.center")},
        {QStringLiteral("我的分享"), QStringLiteral("share")},
    };
    const QVector<MenuItem> &menus = Session::instance().menus();
    for (const QuickEntry &e : allEntries) {
        if (!routeExists(menus, e.route)) {
            continue;
        }
        if (!m_quickScroll) {
            m_quickScroll = new QScrollArea(content);
            m_quickScroll->setObjectName(QStringLiteral("QuickEntryScroll"));
            m_quickScroll->setWidgetResizable(false);
            m_quickScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            m_quickScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            m_quickScroll->setFrameShape(QFrame::NoFrame);
            m_quickScroll->setFixedHeight(80);
            m_quickHost = new QWidget(m_quickScroll);
            m_quickRow = new QHBoxLayout(m_quickHost);
            m_quickRow->setContentsMargins(0, 0, 0, 0);
            m_quickRow->setSpacing(8);
            m_quickRow->setSizeConstraint(QLayout::SetFixedSize);
            m_quickScroll->setWidget(m_quickHost);
            root->addWidget(m_quickScroll);
        }
        auto *btn = makeQuickEntry(e.label, e.route, m_quickHost);
        connect(btn, &QToolButton::clicked, this, [this, route = e.route]() {
            emit navigateRequested(route);
        });
        m_quickRow->addWidget(btn);
    }
    if (m_quickHost) {
        m_quickHost->adjustSize();
        m_quickHost->setFixedSize(qMax(m_quickHost->sizeHint().width(), 80), 72);
    }

    m_status = new QLabel(content);
    m_status->setObjectName(QStringLiteral("StatusLabel"));
    root->addWidget(m_status);

    auto *kpiSection = new QFrame(content);
    kpiSection->setObjectName(QStringLiteral("SectionCard"));
    auto *kpiLayout = new QVBoxLayout(kpiSection);
    kpiLayout->setContentsMargins(20, 16, 20, 16);
    kpiLayout->setSpacing(12);
    m_kpiSectionTitle = new QLabel(QStringLiteral("与我相关"), kpiSection);
    m_kpiSectionTitle->setObjectName(QStringLiteral("SectionTitle"));
    kpiLayout->addWidget(m_kpiSectionTitle);

    auto *kpiGrid = new QGridLayout();
    kpiGrid->setHorizontalSpacing(10);
    kpiGrid->setVerticalSpacing(10);
    for (int i = 0; i < 4; ++i) {
        kpiGrid->addWidget(
            makeKpiTile(QStringLiteral("—"), m_kpiValues[i], m_kpiCaptions[i], kpiSection, i == 1), 0, i);
        kpiGrid->setColumnStretch(i, 1);
    }
    kpiLayout->addLayout(kpiGrid);
    root->addWidget(kpiSection);

    if (m_canHotArticles) {
        m_contentRow = new QWidget(content);
        auto *contentLayout = new QHBoxLayout(m_contentRow);
        contentLayout->setContentsMargins(0, 0, 0, 0);
        contentLayout->setSpacing(16);

        auto *leftCol = new QWidget(m_contentRow);
        m_leftCol = leftCol;
        auto *leftLayout = new QVBoxLayout(leftCol);
        leftLayout->setContentsMargins(0, 0, 0, 0);
        leftLayout->setSpacing(8);

        auto *recHead = new QHBoxLayout();
        auto *recTitle = new QLabel(QStringLiteral("为你推荐"), leftCol);
        recTitle->setObjectName(QStringLiteral("SectionTitle"));
        recHead->addWidget(recTitle);
        recHead->addStretch();
        auto *viewMore = new QPushButton(QStringLiteral("查看更多"), leftCol);
        viewMore->setObjectName(QStringLiteral("TextLink"));
        connect(viewMore, &QPushButton::clicked, this, [this]() {
            emit navigateRequested(QStringLiteral("knowledge.search"));
        });
        recHead->addWidget(viewMore);
        leftLayout->addLayout(recHead);

        m_recommendHint = new QLabel(leftCol);
        m_recommendHint->setObjectName(QStringLiteral("MutedLabel"));
        m_recommendHint->setWordWrap(true);
        leftLayout->addWidget(m_recommendHint);

        auto *feedHost = new QWidget(leftCol);
        m_recommendFeed = new QVBoxLayout(feedHost);
        m_recommendFeed->setContentsMargins(0, 0, 0, 0);
        m_recommendFeed->setSpacing(10);
        leftLayout->addWidget(feedHost);

        m_rightCol = new QFrame(m_contentRow);
        m_rightCol->setObjectName(QStringLiteral("SectionCard"));
        m_rightCol->setMinimumWidth(280);
        auto *rightLayout = new QVBoxLayout(m_rightCol);
        rightLayout->setContentsMargins(16, 16, 16, 16);
        rightLayout->setSpacing(8);
        auto *hotTitle = new QLabel(QStringLiteral("热门知识 TOP 15"), m_rightCol);
        hotTitle->setObjectName(QStringLiteral("SectionTitle"));
        rightLayout->addWidget(hotTitle);

        m_hotScroll = new QScrollArea(m_rightCol);
        m_hotScroll->setObjectName(QStringLiteral("HotRankScroll"));
        m_hotScroll->setWidgetResizable(true);
        m_hotScroll->setFrameShape(QFrame::NoFrame);
        m_hotScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_hotScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_hotListHost = new QWidget(m_hotScroll);
        m_hotRankList = new QVBoxLayout(m_hotListHost);
        m_hotRankList->setContentsMargins(0, 0, 0, 0);
        m_hotRankList->setSpacing(6);
        m_hotScroll->setWidget(m_hotListHost);
        rightLayout->addWidget(m_hotScroll, 1);

        contentLayout->addWidget(leftCol, 2, Qt::AlignTop);
        contentLayout->addWidget(m_rightCol, 1, Qt::AlignTop);
        root->addWidget(m_contentRow, 0, Qt::AlignTop);
    }

    scroll->setWidget(content);
    pageLayout->addWidget(scroll);
}

void DashboardPage::refreshPage() {
    loadMyKpi();
    if (m_canHotArticles) {
        loadRecommendations();
        loadHotArticles();
    }
}

void DashboardPage::loadMyKpi() {
    const auto setTile = [this](int idx, const QString &caption, const QString &value, bool accent = false) {
        if (m_kpiCaptions[idx]) {
            m_kpiCaptions[idx]->setText(caption);
        }
        if (m_kpiValues[idx]) {
            m_kpiValues[idx]->setText(value);
            m_kpiValues[idx]->setObjectName(accent ? QStringLiteral("KpiTileValueAccent")
                                                   : QStringLiteral("KpiTileValue"));
            m_kpiValues[idx]->style()->unpolish(m_kpiValues[idx]);
            m_kpiValues[idx]->style()->polish(m_kpiValues[idx]);
        }
    };

    if (m_canStats) {
        m_kpiAdminMode = true;
        if (m_kpiSectionTitle) {
            m_kpiSectionTitle->setText(QStringLiteral("平台概览"));
        }
        setStatus(QStringLiteral("加载中..."));
        ApiClient::instance().get("/statistics/overview", [this, setTile](const ApiResponse &r) {
            if (!r.ok) {
                setStatus(r.message, true);
                return;
            }
            const QJsonObject o = r.object();
            setTile(0, QStringLiteral("知识总数"), QString::number(asLong(o.value("articleTotal"))));
            setTile(1, QStringLiteral("已上线"), QString::number(asLong(o.value("articleOnline"))), true);
            setTile(2, QStringLiteral("待审核"), QString::number(asLong(o.value("articlePendingAudit"))));
            setTile(3, QStringLiteral("今日检索"), QString::number(asLong(o.value("todaySearches"))));
            setStatus(QString());
        });
        return;
    }

    m_kpiAdminMode = false;
    if (m_kpiSectionTitle) {
        m_kpiSectionTitle->setText(QStringLiteral("与我相关"));
    }

    const qint64 uid = Session::instance().user().id;
    const bool canAudit = Session::instance().hasPermission(QStringLiteral("knowledge:article:audit"));
    const bool canList = Session::instance().hasPermission(QStringLiteral("knowledge:article:list"));

    setStatus(QStringLiteral("加载中..."));

    ApiClient::instance().get("/user/profile/" + QString::number(uid), [this, setTile, uid, canAudit,
                                                                        canList](const ApiResponse &pr) {
        if (!pr.ok) {
            setStatus(pr.message, true);
            return;
        }
        setTile(1, QStringLiteral("我已发布"),
                QString::number(asLong(pr.object().value("onlineArticleCount"))), true);

        ApiClient::instance().get(
            QStringLiteral("/user/profile/%1/favorites?page=1&pageSize=1").arg(uid),
            [setTile](const ApiResponse &fr) {
                if (fr.ok) {
                    setTile(0, QStringLiteral("我的收藏"),
                             QString::number(asLong(fr.object().value("total"))));
                }
            });

        ApiClient::instance().get(QStringLiteral("/knowledge/article/mine?page=1&pageSize=1&status=DRAFT"),
                                  [setTile](const ApiResponse &dr) {
                                      if (dr.ok) {
                                          setTile(2, QStringLiteral("我的草稿"),
                                                   QString::number(asLong(dr.object().value("total"))));
                                      }
                                  });

        if (canAudit && canList) {
            ApiClient::instance().get(
                QStringLiteral("/knowledge/article?status=PENDING_AUDIT&page=1&pageSize=1"),
                [this, setTile](const ApiResponse &ar) {
                    setStatus(QString());
                    if (ar.ok) {
                        setTile(3, QStringLiteral("待我审核"),
                                 QString::number(asLong(ar.object().value("total"))));
                    }
                });
        } else {
            ApiClient::instance().get(
                QStringLiteral("/knowledge/article/mine?page=1&pageSize=1&status=PENDING_AUDIT"),
                [this, setTile](const ApiResponse &ar) {
                    setStatus(QString());
                    if (ar.ok) {
                        setTile(3, QStringLiteral("待审稿件"),
                                 QString::number(asLong(ar.object().value("total"))));
                    }
                });
        }
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
        if (!m_recommendFeed) {
            return;
        }
        clearLayout(m_recommendFeed);

        const QJsonObject data = r.object();
        const QJsonArray list = data.value("items").toArray();
        const QJsonObject summary = data.value("profileSummary").toObject();
        const bool fallback = data.value("fallback").toBool();

        QStringList hintParts;
        for (const QJsonValue &v : summary.value("topTags").toArray()) {
            const QString name = v.toObject().value("name").toString();
            if (!name.isEmpty()) {
                hintParts << name;
            }
        }
        if (hintParts.isEmpty()) {
            for (const QJsonValue &v : summary.value("keywords").toArray()) {
                const QString kw = v.toString();
                if (!kw.isEmpty()) {
                    hintParts << kw;
                }
            }
        }
        if (m_recommendHint) {
            if (hintParts.isEmpty()) {
                m_recommendHint->setText(fallback ? QStringLiteral("根据全站热门为你推荐")
                                                  : QStringLiteral("根据你的兴趣为你推荐"));
            } else {
                QString hint = QStringLiteral("猜你感兴趣：%1").arg(hintParts.join(QStringLiteral(" · ")));
                if (fallback) {
                    hint += QStringLiteral("（热门补足）");
                }
                m_recommendHint->setText(hint);
            }
        }

        for (const QJsonValue &v : list) {
            auto *card = new ArticleFeedCard(v.toObject(), m_contentRow);
            connect(card, &ArticleFeedCard::clicked, this, &DashboardPage::openArticle);
            m_recommendFeed->addWidget(card);
        }
        if (list.isEmpty()) {
            auto *empty = new QLabel(QStringLiteral("暂无推荐内容"), m_contentRow);
            empty->setObjectName(QStringLiteral("MutedLabel"));
            m_recommendFeed->addWidget(empty);
        }
        setStatus(QString());
        syncHotPanelHeight();
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
    ApiClient::instance().get(
        QStringLiteral("/statistics/hot-article?limit=%1").arg(kHotFetchLimit),
        [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        if (!m_hotRankList) {
            return;
        }
        clearLayout(m_hotRankList);

        const QJsonArray list = r.data.toArray();
        int rank = 0;
        for (const QJsonValue &v : list) {
            auto *row = new HotRankRow(++rank, v.toObject(), m_hotListHost);
            row->onClicked = [this](qint64 id) { openArticle(id); };
            m_hotRankList->addWidget(row);
        }
        if (list.isEmpty()) {
            auto *empty = new QLabel(QStringLiteral("暂无热门数据"), m_hotListHost);
            empty->setObjectName(QStringLiteral("MutedLabel"));
            m_hotRankList->addWidget(empty);
        }
        syncHotPanelHeight();
    });
}

void DashboardPage::syncHotPanelHeight() {
    if (!m_leftCol || !m_rightCol) {
        return;
    }
    const auto apply = [this]() {
        if (!m_leftCol || !m_rightCol) {
            return;
        }
        m_leftCol->adjustSize();
        const int target = m_leftCol->sizeHint().height();
        if (target > 0) {
            m_rightCol->setFixedHeight(target);
        }
    };
    apply();
    QTimer::singleShot(0, this, apply);
}

void DashboardPage::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    syncHotPanelHeight();
}

void DashboardPage::openArticle(qint64 articleId) {
    if (articleId <= 0) {
        return;
    }
    if (!Session::instance().hasPermission(QStringLiteral("knowledge:search"))) {
        notify::warn(this, QStringLiteral("当前角色无知识查看权限"));
        return;
    }
    ArticleDetailDialog dlg(articleId, this);
    dlg.exec();
}

void DashboardPage::setStatus(const QString &text, bool error) {
    if (!m_status) {
        return;
    }
    m_status->setText(text);
    m_status->setStyleSheet(error ? QStringLiteral("color:#B94A48;") : QStringLiteral("color:#757575;"));
    if (error && !text.isEmpty()) {
        notify::warn(this, text);
    }
}

} // namespace kb
