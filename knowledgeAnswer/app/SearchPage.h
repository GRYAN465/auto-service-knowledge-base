#pragma once

#include <QJsonArray>
#include <QWidget>

class QFrame;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QStackedWidget;
class QVBoxLayout;

namespace kb {

class ArticleDetailPanel;
class UserProfilePanel;

/**
 * 知识社区页（路由 knowledge.search）：
 *   顶栏搜索 + 最新/最热 + 常用标签 + FeedCard 无限滚动。
 */
class SearchPage : public QWidget {
    Q_OBJECT

public:
    explicit SearchPage(const QString &title, QWidget *parent = nullptr);

private:
    void buildUi();
    void loadTagOptions();
    void rebuildTagButtons();
    void resetFeed();
    void loadMore();
    void setSortHot(bool hot);
    void polishSortButtons();
    void openDetail(qint64 articleId);
    void openProfile(qint64 userId);
    void uploadArticle();

    QString m_title;
    QStackedWidget *m_stack = nullptr;
    QWidget *m_listPage = nullptr;
    ArticleDetailPanel *m_detail = nullptr;
    UserProfilePanel *m_profile = nullptr;

    QLineEdit *m_search = nullptr;
    QPushButton *m_uploadBtn = nullptr;
    QPushButton *m_latestBtn = nullptr;
    QPushButton *m_hotBtn = nullptr;
    QHBoxLayout *m_tagBtnRow = nullptr;
    QVBoxLayout *m_feedLayout = nullptr;
    QScrollArea *m_feedScroll = nullptr;
    QLabel *m_status = nullptr;

    bool m_sortHot = false;
    qint64 m_tagFilter = 0;
    bool m_loading = false;
    bool m_hasMore = true;
    int m_loaded = 0;
    long m_total = 0;
    QJsonArray m_allTags;
};

} // namespace kb
