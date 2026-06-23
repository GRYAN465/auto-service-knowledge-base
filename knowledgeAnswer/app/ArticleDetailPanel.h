#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QTextEdit;
class QVBoxLayout;

namespace kb {

class AvatarLabel;

/** 统一知识详情（社区栈内 / 弹窗共用）：浏览埋点、互动、评论回复、附件下载、作者入口。 */
class ArticleDetailPanel : public QWidget {
    Q_OBJECT

public:
    explicit ArticleDetailPanel(QWidget *parent = nullptr);

    void showArticle(qint64 articleId);
    void setBackButtonVisible(bool visible);

signals:
    void backRequested();
    void authorClicked(qint64 authorId);

private:
    void buildUi();
    void load();
    void refreshState();
    void applyState(const QJsonObject &state);
    void toggleFavorite();
    void doLike(int type);
    void openShare();
    void loadComments();
    void postComment(qint64 parentId, const QString &content);
    void openReplyDialog(qint64 parentId, const QString &targetUserName);
    void downloadAttachment(qint64 attachmentId, const QString &fileName);
    void clearCommentLayout();

    qint64 m_articleId = 0;
    qint64 m_authorId = 0;
    QPushButton *m_backBtn = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_metaLabel = nullptr;
    AvatarLabel *m_authorAvatar = nullptr;
    QPushButton *m_authorBtn = nullptr;
    QWidget *m_tagsHost = nullptr;
    QLabel *m_summaryLabel = nullptr;
    QTextEdit *m_content = nullptr;
    QLabel *m_attachmentTitle = nullptr;
    QVBoxLayout *m_attachmentBox = nullptr;
    QPushButton *m_favBtn = nullptr;
    QPushButton *m_likeBtn = nullptr;
    QPushButton *m_dislikeBtn = nullptr;
    QPushButton *m_shareBtn = nullptr;
    QVBoxLayout *m_commentList = nullptr;
    QLineEdit *m_commentInput = nullptr;
    bool m_favorited = false;
    int m_myLikeType = 0;
};

} // namespace kb
