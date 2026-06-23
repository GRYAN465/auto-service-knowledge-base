#include "app/SearchPage.h"

#include "app/ArticleDetailPanel.h"
#include "app/ArticleFeedCard.h"
#include "app/PinTagsEditDialog.h"
#include "app/UserProfilePanel.h"
#include "core/auth/Session.h"
#include "core/config/PinnedTagsStore.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>

namespace kb {

SearchPage::SearchPage(const QString &title, QWidget *parent)
    : QWidget(parent), m_title(title) {
    buildUi();
    loadTagOptions();
    resetFeed();
}

void SearchPage::buildUi() {
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    m_stack = new QStackedWidget(this);
    m_stack->setObjectName("ContentStack");

    m_listPage = new QWidget(m_stack);
    auto *root = new QVBoxLayout(m_listPage);
    root->setContentsMargins(28, 20, 28, 20);
    root->setSpacing(14);

    auto *topRow = new QHBoxLayout();
    auto *pageTitle = new QLabel(QStringLiteral("知识社区"), m_listPage);
    pageTitle->setObjectName("PlaceholderTitle");
    m_search = new QLineEdit(m_listPage);
    m_search->setPlaceholderText(QStringLiteral("模糊搜索标题/摘要/正文"));
    m_search->setClearButtonEnabled(true);
    m_search->setMinimumWidth(280);
    topRow->addWidget(pageTitle);
    topRow->addStretch();
    topRow->addWidget(m_search, 1);
    root->addLayout(topRow);

    auto *sortRow = new QHBoxLayout();
    m_latestBtn = new QPushButton(QStringLiteral("最新"), m_listPage);
    m_hotBtn = new QPushButton(QStringLiteral("最热"), m_listPage);
    m_latestBtn->setObjectName("PrimaryButton");
    m_hotBtn->setObjectName("GhostButton");
    sortRow->addWidget(m_latestBtn);
    sortRow->addWidget(m_hotBtn);
    sortRow->addStretch();
    root->addLayout(sortRow);
    connect(m_latestBtn, &QPushButton::clicked, this, [this]() { setSortHot(false); resetFeed(); });
    connect(m_hotBtn, &QPushButton::clicked, this, [this]() { setSortHot(true); resetFeed(); });
    connect(m_search, &QLineEdit::returnPressed, this, [this]() { resetFeed(); });

    auto *tagCard = new QFrame(m_listPage);
    tagCard->setObjectName("SectionCard");
    auto *tagCardLayout = new QVBoxLayout(tagCard);
    tagCardLayout->setContentsMargins(16, 14, 16, 14);
    tagCardLayout->setSpacing(10);
    auto *tagHead = new QHBoxLayout();
    auto *tagTitle = new QLabel(QStringLiteral("常用标签"), tagCard);
    tagTitle->setObjectName("SectionTitle");
    auto *editTags = new QPushButton(QStringLiteral("编辑"), tagCard);
    editTags->setObjectName("GhostButton");
    connect(editTags, &QPushButton::clicked, this, [this]() {
        const qint64 uid = Session::instance().user().id;
        QVector<qint64> pinned = PinnedTagsStore::instance().pinnedTagIds(uid);
        PinTagsEditDialog dlg(m_allTags, pinned, this);
        if (dlg.exec() == QDialog::Accepted) {
            PinnedTagsStore::instance().setPinnedTagIds(uid, dlg.selectedTagIds());
            rebuildTagButtons();
        }
    });
    tagHead->addWidget(tagTitle);
    tagHead->addStretch();
    tagHead->addWidget(editTags);
    tagCardLayout->addLayout(tagHead);

    auto *tagBtnHost = new QWidget(tagCard);
    m_tagBtnRow = new QHBoxLayout(tagBtnHost);
    m_tagBtnRow->setContentsMargins(0, 0, 0, 0);
    m_tagBtnRow->setSpacing(8);
    tagCardLayout->addWidget(tagBtnHost);
    root->addWidget(tagCard);

    m_feedScroll = new QScrollArea(m_listPage);
    m_feedScroll->setWidgetResizable(true);
    m_feedScroll->setFrameShape(QFrame::NoFrame);
    auto *feedHost = new QWidget(m_feedScroll);
    m_feedLayout = new QVBoxLayout(feedHost);
    m_feedLayout->setContentsMargins(0, 0, 0, 0);
    m_feedLayout->setSpacing(10);
    m_feedLayout->addStretch();
    m_feedScroll->setWidget(feedHost);
    root->addWidget(m_feedScroll, 1);

    m_status = new QLabel(m_listPage);
    m_status->setObjectName("PageInfoLabel");
    root->addWidget(m_status);

    connect(m_feedScroll->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        QScrollBar *bar = m_feedScroll->verticalScrollBar();
        if (bar->maximum() - value <= 40) {
            loadMore();
        }
    });

    m_detail = new ArticleDetailPanel(m_stack);
    m_stack->addWidget(m_listPage);
    m_stack->addWidget(m_detail);

    outer->addWidget(m_stack);

    connect(m_detail, &ArticleDetailPanel::backRequested, this, [this]() {
        m_stack->setCurrentWidget(m_listPage);
    });
    connect(m_detail, &ArticleDetailPanel::authorClicked, this, &SearchPage::openProfile);
}

void SearchPage::loadTagOptions() {
    ApiClient::instance().get("/knowledge/tag/options", [this](const ApiResponse &r) {
        if (!r.ok) {
            return;
        }
        m_allTags = r.data.toArray();
        const qint64 uid = Session::instance().user().id;
        if (PinnedTagsStore::instance().pinnedTagIds(uid).isEmpty() && m_allTags.size() > 0) {
            QVector<qint64> defaults;
            for (int i = 0; i < qMin(5, m_allTags.size()); ++i) {
                defaults.append(static_cast<qint64>(m_allTags.at(i).toObject().value("id").toDouble()));
            }
            PinnedTagsStore::instance().setPinnedTagIds(uid, defaults);
        }
        rebuildTagButtons();
    });
}

void SearchPage::rebuildTagButtons() {
    while (QLayoutItem *item = m_tagBtnRow->takeAt(0)) {
        if (QWidget *w = item->widget()) w->deleteLater();
        delete item;
    }
    auto *allBtn = new QPushButton(QStringLiteral("全部"), m_listPage);
    allBtn->setObjectName(m_tagFilter == 0 ? "PrimaryButton" : "GhostButton");
    connect(allBtn, &QPushButton::clicked, this, [this]() {
        m_tagFilter = 0;
        rebuildTagButtons();
        resetFeed();
    });
    m_tagBtnRow->addWidget(allBtn);

    const qint64 uid = Session::instance().user().id;
    QVector<qint64> pinned = PinnedTagsStore::instance().pinnedTagIds(uid);
    for (const QJsonValue &v : m_allTags) {
        const QJsonObject t = v.toObject();
        const qint64 id = static_cast<qint64>(t.value("id").toDouble());
        if (!pinned.contains(id)) {
            continue;
        }
        auto *btn = new QPushButton(t.value("name").toString(), m_listPage);
        btn->setObjectName(m_tagFilter == id ? "PrimaryButton" : "GhostButton");
        connect(btn, &QPushButton::clicked, this, [this, id]() {
            m_tagFilter = id;
            rebuildTagButtons();
            resetFeed();
        });
        m_tagBtnRow->addWidget(btn);
    }
    m_tagBtnRow->addStretch();
}

void SearchPage::setSortHot(bool hot) {
    m_sortHot = hot;
    polishSortButtons();
}

void SearchPage::polishSortButtons() {
    m_latestBtn->setObjectName(m_sortHot ? "GhostButton" : "PrimaryButton");
    m_hotBtn->setObjectName(m_sortHot ? "PrimaryButton" : "GhostButton");
    m_latestBtn->style()->unpolish(m_latestBtn);
    m_latestBtn->style()->polish(m_latestBtn);
    m_hotBtn->style()->unpolish(m_hotBtn);
    m_hotBtn->style()->polish(m_hotBtn);
}

void SearchPage::resetFeed() {
    m_loaded = 0;
    m_total = 0;
    m_hasMore = true;
    while (m_feedLayout->count() > 1) {
        QLayoutItem *item = m_feedLayout->takeAt(0);
        if (QWidget *w = item->widget()) w->deleteLater();
        delete item;
    }
    loadMore();
}

void SearchPage::loadMore() {
    if (m_loading || !m_hasMore) {
        return;
    }
    m_loading = true;
    const int pageSize = m_loaded == 0 ? 15 : 10;
    QStringList params;
    params << QStringLiteral("offset=%1").arg(m_loaded);
    params << QStringLiteral("pageSize=%1").arg(pageSize);
    params << QStringLiteral("sortBy=%1").arg(m_sortHot ? "VIEW_COUNT" : "UPDATE_TIME");
    if (!m_search->text().trimmed().isEmpty()) {
        params << "keyword=" + QString::fromUtf8(QUrl::toPercentEncoding(m_search->text().trimmed()));
    }
    if (m_tagFilter > 0) {
        params << QStringLiteral("tagId=%1").arg(m_tagFilter);
    }

    m_status->setText(QStringLiteral("加载中..."));
    QPointer<SearchPage> guard(this);
    ApiClient::instance().get("/search/article?" + params.join('&'), [this, guard, pageSize](const ApiResponse &r) {
        m_loading = false;
        if (!guard) {
            return;
        }
        if (!r.ok) {
            m_status->setText(r.message);
            notify::warn(this, r.message);
            return;
        }
        const QJsonObject d = r.object();
        const QJsonArray list = d.value("list").toArray();
        m_total = static_cast<long>(d.value("total").toDouble());
        for (const QJsonValue &v : list) {
            auto *card = new ArticleFeedCard(v.toObject(), m_listPage);
            connect(card, &ArticleFeedCard::clicked, this, &SearchPage::openDetail);
            m_feedLayout->insertWidget(m_feedLayout->count() - 1, card);
        }
        m_loaded += list.size();
        m_hasMore = m_loaded < m_total && !list.isEmpty();
        if (m_total == 0) {
            m_status->setText(QStringLiteral("暂无数据"));
        } else {
            m_status->setText(QStringLiteral("已加载 %1 / %2 条%3")
                                  .arg(m_loaded)
                                  .arg(m_total)
                                  .arg(m_hasMore ? QStringLiteral("，继续下拉…") : QString()));
        }
    });
}

void SearchPage::openDetail(qint64 articleId) {
    m_detail->showArticle(articleId);
    m_stack->setCurrentWidget(m_detail);
}

void SearchPage::openProfile(qint64 userId) {
    m_profile = new UserProfilePanel(userId, true, m_stack);
    m_stack->addWidget(m_profile);
    connect(m_profile, &UserProfilePanel::backRequested, this, [this]() {
        m_stack->setCurrentWidget(m_detail);
        m_profile->deleteLater();
        m_profile = nullptr;
    });
    connect(m_profile, &UserProfilePanel::openArticle, this, &SearchPage::openDetail);
    m_stack->setCurrentWidget(m_profile);
}

} // namespace kb
