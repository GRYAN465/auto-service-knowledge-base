#include "app/ArticleDetailPanel.h"

#include "app/ShareDialog.h"
#include "common/AvatarLabel.h"
#include "common/TagStyle.h"
#include "core/auth/Session.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QTextEdit>
#include <QVBoxLayout>

#include <functional>

namespace kb {

namespace {

QString typeLabel(const QString &code) {
    if (code == "SCRIPT") return QStringLiteral("话术");
    if (code == "TRAIN") return QStringLiteral("培训");
    if (code == "PRODUCT") return QStringLiteral("产品");
    if (code == "OFFICE") return QStringLiteral("办公");
    return code;
}

QString humanSize(qint64 bytes) {
    if (bytes <= 0) return QStringLiteral("0 B");
    if (bytes < 1024) return QStringLiteral("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
}

QString wrapArticleHtml(const QString &body) {
    return QStringLiteral(
               "<html><head><style>"
               "body{background:#FEFEFE;color:#000000;line-height:1.6;}"
               "a{color:#6B7F74;}"
               "hr{border:none;border-top:1px solid #F1EEE8;}"
               "pre{background:#F3EDE4;border:1px solid #F1EEE8;border-radius:6px;padding:8px 10px;}"
               "</style></head><body>%1</body></html>")
        .arg(body);
}

qint64 asLong(const QJsonValue &v) { return static_cast<qint64>(v.toDouble()); }

QString formatCommentTime(const QString &raw) {
    QString t = raw;
    t.replace('T', ' ');
    return t.length() > 16 ? t.left(16) : t;
}

/** 点击整条评论触发回复。 */
class CommentClickFilter : public QObject {
public:
    using Handler = std::function<void()>;

    CommentClickFilter(QFrame *target, Handler handler, QObject *parent = nullptr)
        : QObject(parent), m_target(target), m_handler(std::move(handler)) {
        m_target->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (obj != m_target || !m_handler) {
            return QObject::eventFilter(obj, event);
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            auto *mouse = static_cast<QMouseEvent *>(event);
            if (mouse->button() == Qt::LeftButton) {
                m_handler();
                return true;
            }
        }
        if (event->type() == QEvent::Enter) {
            m_target->setCursor(Qt::PointingHandCursor);
        } else if (event->type() == QEvent::Leave) {
            m_target->unsetCursor();
        }
        return QObject::eventFilter(obj, event);
    }

private:
    QFrame *m_target = nullptr;
    Handler m_handler;
};

QFrame *buildCommentRow(const QJsonObject &comment, bool isReply, const QString &replyToUserName,
                        bool canReply, const std::function<void(qint64, const QString &)> &onReply,
                        QWidget *parent) {
    const qint64 id = asLong(comment.value("id"));
    const QString userName = comment.value("userName").toString();
    const QString time = formatCommentTime(comment.value("createTime").toString());
    const QString content = comment.value("content").toString();

    auto *row = new QFrame(parent);
    row->setObjectName(isReply ? "CommentReply" : "CommentItem");

    auto *layout = new QVBoxLayout(row);
    layout->setContentsMargins(isReply ? 32 : 0, isReply ? 6 : 10, 0, isReply ? 6 : 10);
    layout->setSpacing(4);

    auto *headRow = new QHBoxLayout();
    headRow->setSpacing(6);

    auto *nameLabel = new QLabel(userName, row);
    nameLabel->setObjectName("CommentUserName");
    headRow->addWidget(nameLabel);

    if (isReply && !replyToUserName.isEmpty()) {
        auto *replyHint = new QLabel(QStringLiteral("回复"), row);
        replyHint->setObjectName("CommentReplyHint");
        headRow->addWidget(replyHint);

        auto *targetLabel = new QLabel(replyToUserName, row);
        targetLabel->setObjectName("CommentReplyTarget");
        headRow->addWidget(targetLabel);
    }

    auto *timeLabel = new QLabel(time, row);
    timeLabel->setObjectName("CommentMeta");
    headRow->addWidget(timeLabel);
    headRow->addStretch();
    layout->addLayout(headRow);

    auto *body = new QLabel(content, row);
    body->setObjectName("CommentBody");
    body->setWordWrap(true);
    body->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(body);

    if (canReply) {
        row->setToolTip(QStringLiteral("点击回复 %1").arg(userName));
        new CommentClickFilter(row, [onReply, id, userName]() { onReply(id, userName); }, row);
    }

    return row;
}

} // namespace

ArticleDetailPanel::ArticleDetailPanel(QWidget *parent) : QWidget(parent) {
    buildUi();
}

void ArticleDetailPanel::setBackButtonVisible(bool visible) {
    if (m_backBtn) {
        m_backBtn->setVisible(visible);
    }
}

void ArticleDetailPanel::buildUi() {
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(28, 20, 28, 20);
    outer->setSpacing(14);

    m_backBtn = new QPushButton(QStringLiteral("← 返回"), this);
    m_backBtn->setObjectName("GhostButton");
    connect(m_backBtn, &QPushButton::clicked, this, &ArticleDetailPanel::backRequested);
    outer->addWidget(m_backBtn, 0, Qt::AlignLeft);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *scrollHost = new QWidget(scroll);
    auto *scrollLayout = new QVBoxLayout(scrollHost);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(14);

    auto *articleCard = new QFrame(scrollHost);
    articleCard->setObjectName("SectionCard");
    auto *articleLayout = new QVBoxLayout(articleCard);
    articleLayout->setContentsMargins(20, 18, 20, 18);
    articleLayout->setSpacing(12);

    m_titleLabel = new QLabel(articleCard);
    m_titleLabel->setObjectName("PlaceholderTitle");
    m_titleLabel->setWordWrap(true);
    articleLayout->addWidget(m_titleLabel);

    m_metaLabel = new QLabel(articleCard);
    m_metaLabel->setObjectName("MutedLabel");
    m_metaLabel->setWordWrap(true);
    articleLayout->addWidget(m_metaLabel);

    auto *authorRow = new QHBoxLayout();
    m_authorAvatar = new AvatarLabel(36, articleCard);
    m_authorBtn = new QPushButton(articleCard);
    m_authorBtn->setObjectName("GhostButton");
    m_authorBtn->setFlat(true);
    m_authorBtn->setCursor(Qt::PointingHandCursor);
    connect(m_authorBtn, &QPushButton::clicked, this, [this]() {
        if (m_authorId > 0) {
            emit authorClicked(m_authorId);
        }
    });
    authorRow->addWidget(m_authorAvatar);
    authorRow->addWidget(m_authorBtn);
    authorRow->addStretch();
    articleLayout->addLayout(authorRow);

    m_tagsHost = new QWidget(articleCard);
    m_tagsHost->setVisible(false);
    articleLayout->addWidget(m_tagsHost);

    auto *bar = new QHBoxLayout();
    bar->setSpacing(8);
    m_favBtn = new QPushButton(QStringLiteral("☆ 收藏"), articleCard);
    m_favBtn->setObjectName("GhostButton");
    m_favBtn->setEnabled(Session::instance().hasPermission("interaction:favorite"));
    connect(m_favBtn, &QPushButton::clicked, this, &ArticleDetailPanel::toggleFavorite);
    m_likeBtn = new QPushButton(QStringLiteral("👍 赞"), articleCard);
    m_likeBtn->setObjectName("GhostButton");
    m_likeBtn->setEnabled(Session::instance().hasPermission("interaction:like"));
    connect(m_likeBtn, &QPushButton::clicked, this, [this]() { doLike(1); });
    m_dislikeBtn = new QPushButton(QStringLiteral("👎 踩"), articleCard);
    m_dislikeBtn->setObjectName("GhostButton");
    m_dislikeBtn->setEnabled(Session::instance().hasPermission("interaction:like"));
    connect(m_dislikeBtn, &QPushButton::clicked, this, [this]() { doLike(-1); });
    m_shareBtn = new QPushButton(QStringLiteral("分享"), articleCard);
    m_shareBtn->setObjectName("GhostButton");
    m_shareBtn->setEnabled(Session::instance().hasPermission("interaction:share"));
    connect(m_shareBtn, &QPushButton::clicked, this, &ArticleDetailPanel::openShare);
    bar->addWidget(m_favBtn);
    bar->addWidget(m_likeBtn);
    bar->addWidget(m_dislikeBtn);
    bar->addWidget(m_shareBtn);
    bar->addStretch();
    articleLayout->addLayout(bar);

    m_summaryLabel = new QLabel(articleCard);
    m_summaryLabel->setObjectName("MutedLabel");
    m_summaryLabel->setWordWrap(true);
    m_summaryLabel->setVisible(false);
    articleLayout->addWidget(m_summaryLabel);

    m_content = new QTextEdit(articleCard);
    m_content->setObjectName("ArticleContent");
    m_content->setReadOnly(true);
    m_content->setMinimumHeight(180);
    articleLayout->addWidget(m_content);

    m_attachmentTitle = new QLabel(QStringLiteral("附件"), articleCard);
    m_attachmentTitle->setObjectName("SectionTitle");
    m_attachmentTitle->setVisible(false);
    articleLayout->addWidget(m_attachmentTitle);

    auto *attachHost = new QWidget(articleCard);
    m_attachmentBox = new QVBoxLayout(attachHost);
    m_attachmentBox->setContentsMargins(0, 0, 0, 0);
    m_attachmentBox->setSpacing(6);
    articleLayout->addWidget(attachHost);

    scrollLayout->addWidget(articleCard);

    auto *commentCard = new QFrame(scrollHost);
    commentCard->setObjectName("SectionCard");
    auto *commentLayout = new QVBoxLayout(commentCard);
    commentLayout->setContentsMargins(20, 18, 20, 18);
    commentLayout->setSpacing(10);

    auto *commentTitle = new QLabel(QStringLiteral("评论"), commentCard);
    commentTitle->setObjectName("SectionTitle");
    commentLayout->addWidget(commentTitle);

    auto *commentHint = new QLabel(QStringLiteral("点击评论即可回复"), commentCard);
    commentHint->setObjectName("MutedLabel");
    commentHint->setVisible(Session::instance().hasPermission("interaction:comment"));
    commentLayout->addWidget(commentHint);

    auto *commentHost = new QWidget(commentCard);
    m_commentList = new QVBoxLayout(commentHost);
    m_commentList->setContentsMargins(0, 0, 0, 0);
    m_commentList->setSpacing(0);
    m_commentList->addStretch();
    commentLayout->addWidget(commentHost);

    auto *inputRow = new QHBoxLayout();
    m_commentInput = new QLineEdit(commentCard);
    m_commentInput->setPlaceholderText(QStringLiteral("写下你的评论……"));
    m_commentInput->setEnabled(Session::instance().hasPermission("interaction:comment"));
    auto *post = new QPushButton(QStringLiteral("发表"), commentCard);
    post->setObjectName("PrimaryButton");
    post->setEnabled(Session::instance().hasPermission("interaction:comment"));
    connect(post, &QPushButton::clicked, this, [this]() {
        const QString text = m_commentInput->text().trimmed();
        if (!text.isEmpty()) {
            postComment(0, text);
        }
    });
    connect(m_commentInput, &QLineEdit::returnPressed, post, &QPushButton::click);
    inputRow->addWidget(m_commentInput, 1);
    inputRow->addWidget(post);
    commentLayout->addLayout(inputRow);

    scrollLayout->addWidget(commentCard);
    scrollLayout->addStretch();
    scroll->setWidget(scrollHost);
    outer->addWidget(scroll, 1);
}

void ArticleDetailPanel::showArticle(qint64 articleId) {
    m_articleId = articleId;
    load();
}

void ArticleDetailPanel::reload() {
    if (m_articleId > 0) {
        load();
    }
}

void ArticleDetailPanel::load() {
    m_titleLabel->setText(QStringLiteral("加载中..."));
    QPointer<ArticleDetailPanel> guard(this);
    ApiClient::instance().get("/knowledge/article/" + QString::number(m_articleId) + "/view",
                              [this, guard](const ApiResponse &r) {
        if (!guard) {
            return;
        }
        if (!r.ok) {
            m_titleLabel->setText(QStringLiteral("加载失败"));
            notify::warn(this, r.message);
            return;
        }
        const QJsonObject d = r.object();
        m_titleLabel->setText(d.value("title").toString());

        QStringList meta;
        const QString category = d.value("categoryName").toString();
        if (!category.isEmpty()) {
            meta << QStringLiteral("分类：%1").arg(category);
        }
        meta << QStringLiteral("类型：%1").arg(typeLabel(d.value("knowledgeType").toString()));
        meta << QStringLiteral("浏览：%1").arg(asLong(d.value("viewCount")));
        const QString onlineTime = d.value("onlineTime").toString().replace('T', ' ');
        if (!onlineTime.isEmpty()) {
            meta << QStringLiteral("上线：%1").arg(onlineTime);
        }
        m_metaLabel->setText(meta.join(QStringLiteral(" · ")));

        m_authorId = asLong(d.value("authorId"));
        const QString authorName = d.value("authorName").toString();
        const QString displayAuthor = authorName.isEmpty() ? d.value("authorUsername").toString() : authorName;
        m_authorAvatar->setDisplayName(displayAuthor);
        m_authorBtn->setText(displayAuthor);

        if (QLayout *old = m_tagsHost->layout()) {
            QLayoutItem *item;
            while ((item = old->takeAt(0)) != nullptr) {
                if (QWidget *w = item->widget()) {
                    w->deleteLater();
                }
                delete item;
            }
            delete old;
        }
        auto *tagLayout = new QHBoxLayout(m_tagsHost);
        tagLayout->setContentsMargins(0, 0, 0, 0);
        tagLayout->setSpacing(6);
        for (const QJsonValue &v : d.value("tags").toArray()) {
            const QJsonObject t = v.toObject();
            tagLayout->addWidget(TagStyle::createTagNameChip(t.value("name").toString(),
                                                             t.value("color").toString(), m_tagsHost));
        }
        tagLayout->addStretch();
        m_tagsHost->setVisible(!d.value("tags").toArray().isEmpty());

        const QString summary = d.value("summary").toString();
        m_summaryLabel->setVisible(!summary.isEmpty());
        m_summaryLabel->setText(summary);
        m_content->setHtml(wrapArticleHtml(d.value("content").toString()));

        while (QLayoutItem *item = m_attachmentBox->takeAt(0)) {
            if (QWidget *w = item->widget()) {
                w->deleteLater();
            }
            delete item;
        }
        const QJsonArray attachments = d.value("attachments").toArray();
        m_attachmentTitle->setVisible(!attachments.isEmpty());
        for (const QJsonValue &v : attachments) {
            const QJsonObject a = v.toObject();
            const qint64 aid = asLong(a.value("id"));
            const QString fileName = a.value("fileName").toString();
            const qint64 fileSize = asLong(a.value("fileSize"));

            auto *rowWidget = new QWidget(this);
            auto *rowLayout = new QHBoxLayout(rowWidget);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            auto *nameLabel = new QLabel(QStringLiteral("%1（%2）").arg(fileName, humanSize(fileSize)), rowWidget);
            nameLabel->setObjectName("MutedLabel");
            auto *dl = new QPushButton(QStringLiteral("下载"), rowWidget);
            dl->setObjectName("GhostButton");
            connect(dl, &QPushButton::clicked, this, [this, aid, fileName]() {
                downloadAttachment(aid, fileName);
            });
            rowLayout->addWidget(nameLabel, 1);
            rowLayout->addWidget(dl);
            m_attachmentBox->addWidget(rowWidget);
        }
    });
    refreshState();
    loadComments();
}

void ArticleDetailPanel::downloadAttachment(qint64 attachmentId, const QString &fileName) {
    const QString savePath = QFileDialog::getSaveFileName(this, QStringLiteral("保存附件"), fileName);
    if (savePath.isEmpty()) {
        return;
    }
    ApiClient::instance().download("/knowledge/attachment/" + QString::number(attachmentId) + "/download",
                                   savePath, [this](bool ok, const QString &error) {
        if (ok) {
            notify::info(this, QStringLiteral("附件已下载"));
        } else {
            notify::warn(this, QStringLiteral("下载失败：%1").arg(error));
        }
    });
}

void ArticleDetailPanel::refreshState() {
    ApiClient::instance().get("/interaction/state?articleId=" + QString::number(m_articleId),
                              [this](const ApiResponse &r) {
        if (r.ok) {
            applyState(r.object());
        }
    });
}

void ArticleDetailPanel::applyState(const QJsonObject &state) {
    m_favorited = state.value("favorited").toBool();
    m_myLikeType = state.value("myLikeType").toInt();
    m_favBtn->setText(m_favorited ? QStringLiteral("★ 已收藏") : QStringLiteral("☆ 收藏"));
    m_likeBtn->setText(QStringLiteral("👍 赞 %1").arg(asLong(state.value("likeCount"))));
    m_dislikeBtn->setText(QStringLiteral("👎 踩 %1").arg(asLong(state.value("dislikeCount"))));

    const auto setActive = [](QPushButton *btn, bool active) {
        btn->setProperty("active", active);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    };
    setActive(m_favBtn, m_favorited);
    setActive(m_likeBtn, m_myLikeType == 1);
    setActive(m_dislikeBtn, m_myLikeType == -1);
}

void ArticleDetailPanel::toggleFavorite() {
    if (m_favorited) {
        ApiClient::instance().del("/interaction/favorite/" + QString::number(m_articleId),
                                  [this](const ApiResponse &r) {
            if (r.ok) {
                refreshState();
            } else {
                notify::warn(this, r.message);
            }
        });
    } else {
        QJsonObject body;
        body.insert("articleId", m_articleId);
        ApiClient::instance().post("/interaction/favorite", body, [this](const ApiResponse &r) {
            if (r.ok) {
                refreshState();
            } else {
                notify::warn(this, r.message);
            }
        });
    }
}

void ArticleDetailPanel::doLike(int type) {
    QJsonObject body;
    body.insert("targetType", "ARTICLE");
    body.insert("targetId", m_articleId);
    body.insert("type", type);
    ApiClient::instance().post("/interaction/like", body, [this](const ApiResponse &r) {
        if (r.ok) {
            applyState(r.object());
        } else {
            notify::warn(this, r.message);
        }
    });
}

void ArticleDetailPanel::openShare() {
    ShareDialog dlg(m_articleId, this);
    dlg.exec();
}

void ArticleDetailPanel::clearCommentLayout() {
    if (!m_commentList) {
        return;
    }
    while (QLayoutItem *item = m_commentList->takeAt(0)) {
        if (QWidget *w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
}

void ArticleDetailPanel::openReplyDialog(qint64 parentId, const QString &targetUserName) {
    if (!Session::instance().hasPermission("interaction:comment")) {
        return;
    }
    bool ok = false;
    const QString text = QInputDialog::getMultiLineText(
        this,
        QStringLiteral("回复 %1").arg(targetUserName),
        QStringLiteral("回复内容："),
        QString(),
        &ok);
    if (ok && !text.trimmed().isEmpty()) {
        postComment(parentId, text.trimmed());
    }
}

void ArticleDetailPanel::loadComments() {
    ApiClient::instance().get("/interaction/comment?articleId=" + QString::number(m_articleId),
                              [this](const ApiResponse &r) {
        clearCommentLayout();
        const bool canReply = Session::instance().hasPermission("interaction:comment");
        const auto onReply = [this](qint64 id, const QString &userName) {
            openReplyDialog(id, userName);
        };

        if (!r.ok) {
            m_commentList->addStretch();
            return;
        }
        const QJsonArray roots = r.data.toArray();
        if (roots.isEmpty()) {
            auto *empty = new QLabel(QStringLiteral("暂无评论，来抢沙发～"), this);
            empty->setObjectName("MutedLabel");
            empty->setAlignment(Qt::AlignCenter);
            empty->setContentsMargins(0, 24, 0, 24);
            m_commentList->addWidget(empty);
            m_commentList->addStretch();
            return;
        }

        std::function<void(const QJsonObject &, bool, const QString &, QVBoxLayout *)> renderReplies;
        renderReplies = [&](const QJsonObject &parent, bool /*unused*/, const QString &parentName,
                            QVBoxLayout *container) {
            for (const QJsonValue &childVal : parent.value("children").toArray()) {
                const QJsonObject child = childVal.toObject();
                container->addWidget(buildCommentRow(child, true, parentName, canReply, onReply, this));
                renderReplies(child, true, child.value("userName").toString(), container);
            }
        };

        for (const QJsonValue &v : roots) {
            const QJsonObject rootComment = v.toObject();

            auto *thread = new QFrame(this);
            thread->setObjectName("CommentThread");
            auto *threadLayout = new QVBoxLayout(thread);
            threadLayout->setContentsMargins(0, 0, 0, 0);
            threadLayout->setSpacing(0);

            threadLayout->addWidget(buildCommentRow(rootComment, false, QString(), canReply, onReply, thread));
            renderReplies(rootComment, false, rootComment.value("userName").toString(), threadLayout);

            m_commentList->addWidget(thread);
        }
        m_commentList->addStretch();
    });
}

void ArticleDetailPanel::postComment(qint64 parentId, const QString &content) {
    QJsonObject body;
    body.insert("articleId", m_articleId);
    if (parentId > 0) {
        body.insert("parentId", parentId);
    }
    body.insert("content", content);
    ApiClient::instance().post("/interaction/comment", body, [this, parentId](const ApiResponse &r) {
        if (!r.ok) {
            notify::warn(this, r.message);
            return;
        }
        if (parentId == 0) {
            m_commentInput->clear();
        }
        loadComments();
    });
}

} // namespace kb
