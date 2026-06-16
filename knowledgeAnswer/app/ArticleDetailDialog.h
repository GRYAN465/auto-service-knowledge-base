#pragma once

#include <QDialog>

class QJsonObject;
class QLabel;
class QLineEdit;
class QPushButton;
class QTextEdit;
class QVBoxLayout;

namespace kb {

/**
 * 知识详情只读弹窗（模块 5 + 模块 6 互动）：
 *   - 打开即 GET /knowledge/article/{id}/view（这一次调用即完成浏览埋点 + view_count++）
 *   - 渲染标题 / 一行 meta（分类·类型·作者·浏览·上线时间）/ 彩色标签 / 摘要 / 富文本正文 / 附件下载
 *   - 互动条（模块 6）：收藏 toggle / 点赞(计数) / 点踩(计数) / 分享，初态与计数来自 GET /interaction/state
 *   - 评论区（模块 6）：GET /interaction/comment 树形展示 + 发表/回复（POST /interaction/comment）
 * 全部复用既有 objectName（PrimaryButton/GhostButton/DataTable/StatusLabel），按权限码控制按钮可用。
 */
class ArticleDetailDialog : public QDialog {
    Q_OBJECT

public:
    explicit ArticleDetailDialog(qint64 articleId, QWidget *parent = nullptr);

private:
    void buildUi();
    void load();
    void downloadAttachment(qint64 attachmentId, const QString &fileName);

    // 模块 6 互动
    void refreshState();
    void applyState(const QJsonObject &state);
    void toggleFavorite();
    void doLike(int type);
    void openShare();
    void loadComments();
    void postComment(qint64 parentId, const QString &content);

    qint64 m_articleId = 0;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_metaLabel = nullptr;
    QLabel *m_tagsLabel = nullptr;
    QLabel *m_summaryLabel = nullptr;
    QTextEdit *m_content = nullptr;
    QVBoxLayout *m_attachmentBox = nullptr;
    QLabel *m_attachmentTitle = nullptr;

    // 互动条
    QPushButton *m_favBtn = nullptr;
    QPushButton *m_likeBtn = nullptr;
    QPushButton *m_dislikeBtn = nullptr;
    QPushButton *m_shareBtn = nullptr;
    bool m_favorited = false;
    int m_myLikeType = 0;

    // 评论区
    QLabel *m_commentTitle = nullptr;
    QVBoxLayout *m_commentList = nullptr;
    QLineEdit *m_commentInput = nullptr;
    QPushButton *m_commentPost = nullptr;
};

} // namespace kb
