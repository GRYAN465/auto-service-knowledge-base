#include "app/UserProfilePanel.h"

#include "app/ArticleFeedCard.h"
#include "app/ChangePasswordDialog.h"
#include "common/AvatarLabel.h"
#include "core/auth/Session.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace kb {

namespace {

QPushButton *makeSubTab(const QString &text, QWidget *parent) {
    auto *btn = new QPushButton(text, parent);
    btn->setObjectName("SubTabButton");
    btn->setCheckable(true);
    btn->setAutoExclusive(true);
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

QScrollArea *makeFeedScroll(QVBoxLayout **feedLayout, QWidget *parent) {
    auto *scroll = new QScrollArea(parent);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *host = new QWidget(scroll);
    *feedLayout = new QVBoxLayout(host);
    (*feedLayout)->setContentsMargins(0, 0, 0, 0);
    (*feedLayout)->setSpacing(10);
    (*feedLayout)->addStretch();
    scroll->setWidget(host);
    return scroll;
}

} // namespace

UserProfilePanel::UserProfilePanel(qint64 userId, bool showBack, QWidget *parent)
    : QWidget(parent), m_userId(userId), m_showBack(showBack) {
    buildUi();
    reload();
}

void UserProfilePanel::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(28, 20, 28, 20);
    root->setSpacing(14);

    if (m_showBack) {
        auto *back = new QPushButton(QStringLiteral("← 返回"), this);
        back->setObjectName("GhostButton");
        connect(back, &QPushButton::clicked, this, &UserProfilePanel::backRequested);
        root->addWidget(back, 0, Qt::AlignLeft);
    }

    auto *headCard = new QFrame(this);
    headCard->setObjectName("SectionCard");
    auto *headLayout = new QHBoxLayout(headCard);
    headLayout->setContentsMargins(20, 18, 20, 18);
    headLayout->setSpacing(16);

    m_avatar = new AvatarLabel(56, headCard);
    auto *infoCol = new QVBoxLayout();
    infoCol->setSpacing(6);
    m_nameLabel = new QLabel(headCard);
    m_nameLabel->setObjectName("PlaceholderTitle");
    m_metaLabel = new QLabel(headCard);
    m_metaLabel->setObjectName("MutedLabel");
    infoCol->addWidget(m_nameLabel);
    infoCol->addWidget(m_metaLabel);

    m_changePwdBtn = new QPushButton(QStringLiteral("修改密码"), headCard);
    m_changePwdBtn->setObjectName("GhostButton");
    connect(m_changePwdBtn, &QPushButton::clicked, this, [this]() {
        ChangePasswordDialog dlg(this);
        dlg.exec();
    });
    m_privacyBox = new QCheckBox(QStringLiteral("收藏仅自己可见"), headCard);
    connect(m_privacyBox, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_self) {
            return;
        }
        QJsonObject body;
        body.insert("favoritePrivate", checked);
        ApiClient::instance().put("/user/profile/favorite-privacy", body, [this](const ApiResponse &r) {
            if (!r.ok) {
                notify::warn(this, r.message);
                m_privacyBox->blockSignals(true);
                m_privacyBox->setChecked(!m_privacyBox->isChecked());
                m_privacyBox->blockSignals(false);
            }
        });
    });
    infoCol->addWidget(m_changePwdBtn);
    infoCol->addWidget(m_privacyBox);

    headLayout->addWidget(m_avatar);
    headLayout->addLayout(infoCol, 1);
    root->addWidget(headCard);

    auto *bodyCard = new QFrame(this);
    bodyCard->setObjectName("SectionCard");
    auto *bodyLayout = new QVBoxLayout(bodyCard);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    auto *tabBarHost = new QWidget(bodyCard);
    tabBarHost->setObjectName("SubTabBar");
    auto *tabBar = new QHBoxLayout(tabBarHost);
    tabBar->setContentsMargins(16, 12, 16, 0);
    tabBar->setSpacing(8);
    m_pubTab = makeSubTab(QStringLiteral("发布的知识"), tabBarHost);
    m_favTab = makeSubTab(QStringLiteral("收藏的知识"), tabBarHost);
    m_draftTab = makeSubTab(QStringLiteral("我的草稿"), tabBarHost);
    m_draftTab->setVisible(false);
    m_pubTab->setChecked(true);
    tabBar->addWidget(m_pubTab);
    tabBar->addWidget(m_favTab);
    tabBar->addWidget(m_draftTab);
    tabBar->addStretch();
    bodyLayout->addWidget(tabBarHost);

    m_tabStack = new QStackedWidget(bodyCard);
    m_tabStack->setObjectName("SubTabPane");

    auto *pubPage = new QWidget(m_tabStack);
    auto *pubLayout = new QVBoxLayout(pubPage);
    pubLayout->setContentsMargins(16, 12, 16, 16);
    m_pubEmpty = new QLabel(QStringLiteral("暂无发布的知识"), pubPage);
    m_pubEmpty->setObjectName("MutedLabel");
    m_pubEmpty->setAlignment(Qt::AlignCenter);
    m_pubScroll = makeFeedScroll(&m_pubFeed, pubPage);
    pubLayout->addWidget(m_pubEmpty);
    pubLayout->addWidget(m_pubScroll, 1);

    auto *favPage = new QWidget(m_tabStack);
    auto *favLayoutOuter = new QVBoxLayout(favPage);
    favLayoutOuter->setContentsMargins(16, 12, 16, 16);
    m_favEmpty = new QLabel(QStringLiteral("暂无收藏的知识"), favPage);
    m_favEmpty->setObjectName("MutedLabel");
    m_favEmpty->setAlignment(Qt::AlignCenter);
    m_favScroll = makeFeedScroll(&m_favFeed, favPage);
    favLayoutOuter->addWidget(m_favEmpty);
    favLayoutOuter->addWidget(m_favScroll, 1);

    m_tabStack->addWidget(pubPage);
    m_tabStack->addWidget(favPage);

    auto *draftPage = new QWidget(m_tabStack);
    auto *draftLayoutOuter = new QVBoxLayout(draftPage);
    draftLayoutOuter->setContentsMargins(16, 12, 16, 16);
    m_draftEmpty = new QLabel(QStringLiteral("暂无草稿"), draftPage);
    m_draftEmpty->setObjectName("MutedLabel");
    m_draftEmpty->setAlignment(Qt::AlignCenter);
    m_draftScroll = makeFeedScroll(&m_draftFeed, draftPage);
    draftLayoutOuter->addWidget(m_draftEmpty);
    draftLayoutOuter->addWidget(m_draftScroll, 1);
    m_tabStack->addWidget(draftPage);

    bodyLayout->addWidget(m_tabStack, 1);
    root->addWidget(bodyCard, 1);

    connect(m_pubTab, &QPushButton::clicked, this, [this]() { switchTab(0); });
    connect(m_favTab, &QPushButton::clicked, this, [this]() { switchTab(1); });
    connect(m_draftTab, &QPushButton::clicked, this, [this]() { switchTab(2); });
}

void UserProfilePanel::reload() {
    loadProfile();
}

bool UserProfilePanel::isSelfProfile() const {
    const qint64 selfId = Session::instance().user().id;
    return m_userId == 0 || m_userId == selfId;
}

qint64 UserProfilePanel::profileUserId() const {
    return m_userId > 0 ? m_userId : Session::instance().user().id;
}

void UserProfilePanel::loadProfile() {
    const qint64 id = m_userId > 0 ? m_userId : Session::instance().user().id;
    QPointer<UserProfilePanel> guard(this);
    ApiClient::instance().get("/user/profile/" + QString::number(id), [this, guard](const ApiResponse &r) {
        if (!guard || !r.ok) {
            if (guard && !r.ok) {
                notify::warn(this, r.message);
            }
            return;
        }
        const QJsonObject d = r.object();
        m_self = d.value("self").toBool(true);
        m_favoritesVisible = d.value("favoritesVisible").toBool(true);
        m_favoritePrivate = d.value("favoritePrivate").toBool(false);

        const QString realName = d.value("realName").toString();
        const QString username = d.value("username").toString();
        m_avatar->setDisplayName(realName.isEmpty() ? username : realName);
        m_nameLabel->setText(realName.isEmpty() ? username : realName);
        m_metaLabel->setText(QStringLiteral("账号：%1 · 已上线知识 %2 篇")
                                 .arg(username)
                                 .arg(static_cast<qint64>(d.value("onlineArticleCount").toDouble())));

        m_changePwdBtn->setVisible(m_self);
        m_privacyBox->setVisible(m_self);
        m_draftTab->setVisible(m_self);
        m_favTab->setVisible(true);
        if (m_self) {
            m_privacyBox->blockSignals(true);
            m_privacyBox->setChecked(m_favoritePrivate);
            m_privacyBox->blockSignals(false);
        }

        loadPublished();
        loadFavorites();
        if (m_self) {
            loadDrafts();
        }
    });
}

void UserProfilePanel::clearFeed(QVBoxLayout *layout) {
    while (layout->count() > 1) {
        QLayoutItem *item = layout->takeAt(0);
        if (QWidget *w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
}

void UserProfilePanel::appendFeedCards(const QJsonArray &list, QVBoxLayout *layout, bool &hasMore,
                                       bool draftMode) {
    hasMore = !list.isEmpty();
    for (const QJsonValue &v : list) {
        auto *card = new ArticleFeedCard(v.toObject(), this);
        if (draftMode) {
            connect(card, &ArticleFeedCard::clicked, this, &UserProfilePanel::openDraft);
        } else {
            connect(card, &ArticleFeedCard::clicked, this, &UserProfilePanel::openArticle);
        }
        layout->insertWidget(layout->count() - 1, card);
    }
}

void UserProfilePanel::loadPublished() {
    clearFeed(m_pubFeed);
    const qint64 id = profileUserId();
    QPointer<UserProfilePanel> guard(this);

    if (isSelfProfile()) {
        ApiClient::instance().get("/knowledge/article/mine?page=1&pageSize=50&status=ONLINE",
                                  [this, guard](const ApiResponse &r) {
            if (!guard) {
                return;
            }
            if (!r.ok) {
                notify::warn(this, r.message);
                return;
            }
            const QJsonArray list = r.object().value("list").toArray();
            bool has = false;
            appendFeedCards(list, m_pubFeed, has);
            m_pubEmpty->setVisible(!has);
            m_pubScroll->setVisible(has);
        });
    } else {
        ApiClient::instance().get("/search/article?authorId=" + QString::number(id)
                                      + "&offset=0&pageSize=50&sortBy=UPDATE_TIME",
                                  [this, guard](const ApiResponse &r) {
            if (!guard) {
                return;
            }
            if (!r.ok) {
                notify::warn(this, r.message);
                return;
            }
            const QJsonArray list = r.object().value("list").toArray();
            bool has = false;
            appendFeedCards(list, m_pubFeed, has);
            m_pubEmpty->setText(has ? QString() : QStringLiteral("暂无已上线知识"));
            m_pubEmpty->setVisible(!has);
            m_pubScroll->setVisible(has);
        });
    }
}

void UserProfilePanel::loadFavorites() {
    clearFeed(m_favFeed);
    const qint64 id = profileUserId();
    QPointer<UserProfilePanel> guard(this);

    ApiClient::instance().get(QStringLiteral("/user/profile/%1/favorites?page=1&pageSize=50").arg(id),
                              [this, guard, id](const ApiResponse &r) {
        if (!guard) {
            return;
        }
        if (!r.ok) {
            if (r.code == 2003 || r.message.contains(QStringLiteral("隐私"))) {
                m_favEmpty->setText(QStringLiteral("对方已将收藏设为隐私"));
            } else {
                notify::warn(this, r.message);
                m_favEmpty->setText(QStringLiteral("加载失败"));
            }
            m_favEmpty->setVisible(true);
            m_favScroll->setVisible(false);
            return;
        }

        QJsonArray list = r.object().value("list").toArray();

        bool has = false;
        appendFeedCards(list, m_favFeed, has);
        if (isSelfProfile()) {
            m_favEmpty->setText(QStringLiteral("暂无收藏的知识"));
        } else {
            m_favEmpty->setText(QStringLiteral("对方暂无收藏"));
        }
        m_favEmpty->setVisible(!has);
        m_favScroll->setVisible(has);
    });
}

void UserProfilePanel::loadDrafts() {
    if (!m_self) {
        return;
    }
    clearFeed(m_draftFeed);
    QPointer<UserProfilePanel> guard(this);
    ApiClient::instance().get("/knowledge/article/mine?page=1&pageSize=50&status=DRAFT",
                              [this, guard](const ApiResponse &r) {
        if (!guard) {
            return;
        }
        if (!r.ok) {
            notify::warn(this, r.message);
            return;
        }
        const QJsonArray list = r.object().value("list").toArray();
        bool has = false;
        appendFeedCards(list, m_draftFeed, has, true);
        m_draftEmpty->setVisible(!has);
        m_draftScroll->setVisible(has);
    });
}

void UserProfilePanel::switchTab(int index) {
    m_tabStack->setCurrentIndex(index);
    m_pubTab->setChecked(index == 0);
    m_favTab->setChecked(index == 1);
    if (m_draftTab) {
        m_draftTab->setChecked(index == 2);
    }
}

} // namespace kb
