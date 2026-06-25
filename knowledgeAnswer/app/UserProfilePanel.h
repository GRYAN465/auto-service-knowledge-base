#pragma once

#include <QWidget>

class QCheckBox;
class QFrame;
class QLabel;
class QPushButton;
class QScrollArea;
class QStackedWidget;
class QVBoxLayout;

namespace kb {

class AvatarLabel;
class ArticleFeedCard;

/**
 * 用户主页（本人 userId=0 / 他人 userId>0），SubTab：发布 / 收藏 / 草稿（仅本人）。
 */
class UserProfilePanel : public QWidget {
    Q_OBJECT

public:
    explicit UserProfilePanel(qint64 userId = 0, bool showBack = false, QWidget *parent = nullptr);

    void reload();

signals:
    void backRequested();
    void openArticle(qint64 articleId);
    void openDraft(qint64 articleId);

private:
    void buildUi();
    void loadProfile();
    void loadPublished();
    void loadFavorites();
    void loadDrafts();
    bool isSelfProfile() const;
    qint64 profileUserId() const;
    void switchTab(int index);
    void appendFeedCards(const QJsonArray &list, QVBoxLayout *layout, bool &hasMore,
                         bool draftMode = false);
    void clearFeed(QVBoxLayout *layout);

    qint64 m_userId = 0;
    bool m_showBack = false;
    bool m_self = false;
    bool m_favoritesVisible = true;
    bool m_favoritePrivate = false;

    AvatarLabel *m_avatar = nullptr;
    QLabel *m_nameLabel = nullptr;
    QLabel *m_metaLabel = nullptr;
    QPushButton *m_changePwdBtn = nullptr;
    QCheckBox *m_privacyBox = nullptr;
    QPushButton *m_pubTab = nullptr;
    QPushButton *m_favTab = nullptr;
    QPushButton *m_draftTab = nullptr;
    QStackedWidget *m_tabStack = nullptr;
    QVBoxLayout *m_pubFeed = nullptr;
    QVBoxLayout *m_favFeed = nullptr;
    QVBoxLayout *m_draftFeed = nullptr;
    QLabel *m_pubEmpty = nullptr;
    QLabel *m_favEmpty = nullptr;
    QLabel *m_draftEmpty = nullptr;
    QScrollArea *m_pubScroll = nullptr;
    QScrollArea *m_favScroll = nullptr;
    QScrollArea *m_draftScroll = nullptr;
};

} // namespace kb
