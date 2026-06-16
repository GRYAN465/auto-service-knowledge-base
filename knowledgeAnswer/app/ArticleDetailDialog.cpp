#include "app/ArticleDetailDialog.h"

#include "app/ShareDialog.h"
#include "core/auth/Session.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
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

qint64 asLong(const QJsonValue &v) { return static_cast<qint64>(v.toDouble()); }

// 清空一个布局内的全部子部件（重拉评论时复用）。
void clearLayout(QVBoxLayout *layout) {
    if (!layout) return;
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
}

} // namespace

ArticleDetailDialog::ArticleDetailDialog(qint64 articleId, QWidget *parent)
    : QDialog(parent), m_articleId(articleId) {
    setWindowTitle(QStringLiteral("知识详情"));
    resize(760, 720);
    buildUi();
    load();
}

void ArticleDetailDialog::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(10);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setStyleSheet("font-size:20px; font-weight:600; color:#111827;");
    root->addWidget(m_titleLabel);

    m_metaLabel = new QLabel(this);
    m_metaLabel->setStyleSheet("color:#6B7280;");
    root->addWidget(m_metaLabel);

    m_tagsLabel = new QLabel(this);
    m_tagsLabel->setTextFormat(Qt::RichText);
    m_tagsLabel->setWordWrap(true);
    root->addWidget(m_tagsLabel);

    // 互动条：收藏 / 点赞 / 点踩 / 分享（按权限码控制可用）。
    auto *bar = new QHBoxLayout();
    bar->setSpacing(8);
    m_favBtn = new QPushButton(QStringLiteral("☆ 收藏"), this);
    m_favBtn->setObjectName("GhostButton");
    m_favBtn->setEnabled(Session::instance().hasPermission("interaction:favorite"));
    connect(m_favBtn, &QPushButton::clicked, this, &ArticleDetailDialog::toggleFavorite);
    m_likeBtn = new QPushButton(QStringLiteral("👍 赞"), this);
    m_likeBtn->setObjectName("GhostButton");
    m_likeBtn->setEnabled(Session::instance().hasPermission("interaction:like"));
    connect(m_likeBtn, &QPushButton::clicked, this, [this]() { doLike(1); });
    m_dislikeBtn = new QPushButton(QStringLiteral("👎 踩"), this);
    m_dislikeBtn->setObjectName("GhostButton");
    m_dislikeBtn->setEnabled(Session::instance().hasPermission("interaction:like"));
    connect(m_dislikeBtn, &QPushButton::clicked, this, [this]() { doLike(-1); });
    m_shareBtn = new QPushButton(QStringLiteral("分享"), this);
    m_shareBtn->setObjectName("GhostButton");
    m_shareBtn->setEnabled(Session::instance().hasPermission("interaction:share"));
    connect(m_shareBtn, &QPushButton::clicked, this, &ArticleDetailDialog::openShare);
    bar->addWidget(m_favBtn);
    bar->addWidget(m_likeBtn);
    bar->addWidget(m_dislikeBtn);
    bar->addWidget(m_shareBtn);
    bar->addStretch();
    root->addLayout(bar);

    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setWordWrap(true);
    m_summaryLabel->setStyleSheet("color:#374151; background:#F9FAFB; padding:8px; border-radius:6px;");
    m_summaryLabel->setVisible(false);
    root->addWidget(m_summaryLabel);

    m_content = new QTextEdit(this);
    m_content->setObjectName("DataTable");
    m_content->setReadOnly(true);
    root->addWidget(m_content, 3);

    // 附件区（标题 + 动态行）。
    m_attachmentTitle = new QLabel(QStringLiteral("附件"), this);
    m_attachmentTitle->setStyleSheet("font-weight:600; color:#111827;");
    m_attachmentTitle->setVisible(false);
    root->addWidget(m_attachmentTitle);

    auto *attachmentHost = new QWidget(this);
    m_attachmentBox = new QVBoxLayout(attachmentHost);
    m_attachmentBox->setContentsMargins(0, 0, 0, 0);
    m_attachmentBox->setSpacing(6);
    root->addWidget(attachmentHost);

    // 评论区：标题 + 滚动列表 + 输入行。
    m_commentTitle = new QLabel(QStringLiteral("评论"), this);
    m_commentTitle->setStyleSheet("font-weight:600; color:#111827;");
    root->addWidget(m_commentTitle);

    auto *commentScroll = new QScrollArea(this);
    commentScroll->setWidgetResizable(true);
    commentScroll->setFrameShape(QFrame::NoFrame);
    commentScroll->setMinimumHeight(140);
    auto *commentHost = new QWidget(commentScroll);
    m_commentList = new QVBoxLayout(commentHost);
    m_commentList->setContentsMargins(0, 0, 0, 0);
    m_commentList->setSpacing(8);
    m_commentList->addStretch();
    commentScroll->setWidget(commentHost);
    root->addWidget(commentScroll, 2);

    auto *inputRow = new QHBoxLayout();
    m_commentInput = new QLineEdit(this);
    m_commentInput->setPlaceholderText(QStringLiteral("写下你的评论……"));
    m_commentInput->setEnabled(Session::instance().hasPermission("interaction:comment"));
    m_commentPost = new QPushButton(QStringLiteral("发表"), this);
    m_commentPost->setObjectName("PrimaryButton");
    m_commentPost->setEnabled(Session::instance().hasPermission("interaction:comment"));
    connect(m_commentPost, &QPushButton::clicked, this, [this]() {
        const QString text = m_commentInput->text().trimmed();
        if (text.isEmpty()) {
            return;
        }
        postComment(0, text);
    });
    connect(m_commentInput, &QLineEdit::returnPressed, m_commentPost, &QPushButton::click);
    inputRow->addWidget(m_commentInput, 1);
    inputRow->addWidget(m_commentPost);
    root->addLayout(inputRow);

    auto *footer = new QHBoxLayout();
    footer->addStretch();
    auto *close = new QPushButton(QStringLiteral("关闭"), this);
    close->setObjectName("GhostButton");
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    footer->addWidget(close);
    root->addLayout(footer);
}

void ArticleDetailDialog::load() {
    m_titleLabel->setText(QStringLiteral("加载中..."));
    ApiClient::instance().get("/knowledge/article/" + QString::number(m_articleId) + "/view",
                              [this](const ApiResponse &r) {
        if (!r.ok) {
            m_titleLabel->setText(QStringLiteral("加载失败"));
            notify::warn(this, r.message);
            return;
        }
        const QJsonObject d = r.object();
        m_titleLabel->setText(d.value("title").toString());

        QStringList meta;
        const QString category = d.value("categoryName").toString();
        if (!category.isEmpty()) meta << QStringLiteral("分类：%1").arg(category);
        meta << QStringLiteral("类型：%1").arg(typeLabel(d.value("knowledgeType").toString()));
        const QString author = d.value("authorName").toString();
        if (!author.isEmpty()) meta << QStringLiteral("作者：%1").arg(author);
        meta << QStringLiteral("浏览：%1").arg(asLong(d.value("viewCount")));
        const QString onlineTime = d.value("onlineTime").toString().replace('T', ' ');
        if (!onlineTime.isEmpty()) meta << QStringLiteral("上线：%1").arg(onlineTime);
        m_metaLabel->setText(meta.join(QStringLiteral("　·　")));

        // 彩色标签
        const QJsonArray tags = d.value("tags").toArray();
        if (tags.isEmpty()) {
            m_tagsLabel->setVisible(false);
        } else {
            QStringList chips;
            for (const QJsonValue &v : tags) {
                const QJsonObject t = v.toObject();
                QString color = t.value("color").toString();
                if (color.isEmpty()) color = QStringLiteral("#6B7280");
                chips << QStringLiteral("<span style='background:%1; color:#FFFFFF; padding:2px 8px; "
                                        "border-radius:8px;'>%2</span>")
                             .arg(color, t.value("name").toString().toHtmlEscaped());
            }
            m_tagsLabel->setText(chips.join(QStringLiteral("&nbsp;&nbsp;")));
            m_tagsLabel->setVisible(true);
        }

        const QString summary = d.value("summary").toString();
        if (summary.isEmpty()) {
            m_summaryLabel->setVisible(false);
        } else {
            m_summaryLabel->setText(summary);
            m_summaryLabel->setVisible(true);
        }

        m_content->setHtml(d.value("content").toString());

        // 附件行
        const QJsonArray attachments = d.value("attachments").toArray();
        m_attachmentTitle->setVisible(!attachments.isEmpty());
        for (const QJsonValue &v : attachments) {
            const QJsonObject a = v.toObject();
            const qint64 attachmentId = asLong(a.value("id"));
            const QString fileName = a.value("fileName").toString();
            const qint64 fileSize = asLong(a.value("fileSize"));

            auto *rowWidget = new QWidget(this);
            auto *rowLayout = new QHBoxLayout(rowWidget);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            auto *nameLabel = new QLabel(QStringLiteral("%1（%2）").arg(fileName, humanSize(fileSize)), rowWidget);
            nameLabel->setStyleSheet("color:#374151;");
            auto *dl = new QPushButton(QStringLiteral("下载"), rowWidget);
            dl->setObjectName("GhostButton");
            connect(dl, &QPushButton::clicked, this,
                    [this, attachmentId, fileName]() { downloadAttachment(attachmentId, fileName); });
            rowLayout->addWidget(nameLabel, 1);
            rowLayout->addWidget(dl);
            m_attachmentBox->addWidget(rowWidget);
        }
    });

    // 模块 6：互动初态 + 评论列表。
    refreshState();
    loadComments();
}

void ArticleDetailDialog::downloadAttachment(qint64 attachmentId, const QString &fileName) {
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

// ---------------------------------------------------------------- 模块 6 互动

void ArticleDetailDialog::refreshState() {
    ApiClient::instance().get("/interaction/state?articleId=" + QString::number(m_articleId),
                              [this](const ApiResponse &r) {
        if (r.ok) {
            applyState(r.object());
        }
    });
}

void ArticleDetailDialog::applyState(const QJsonObject &state) {
    m_favorited = state.value("favorited").toBool();
    m_myLikeType = state.value("myLikeType").toInt();
    const qint64 likeCount = asLong(state.value("likeCount"));
    const qint64 dislikeCount = asLong(state.value("dislikeCount"));

    m_favBtn->setText(m_favorited ? QStringLiteral("★ 已收藏") : QStringLiteral("☆ 收藏"));
    m_likeBtn->setText(QStringLiteral("👍 赞 %1").arg(likeCount));
    m_dislikeBtn->setText(QStringLiteral("👎 踩 %1").arg(dislikeCount));
    // 用加粗体现我当前的表态，避免依赖额外样式。
    m_likeBtn->setStyleSheet(m_myLikeType == 1 ? "font-weight:600;" : "");
    m_dislikeBtn->setStyleSheet(m_myLikeType == -1 ? "font-weight:600;" : "");
}

void ArticleDetailDialog::toggleFavorite() {
    if (m_favorited) {
        ApiClient::instance().del("/interaction/favorite/" + QString::number(m_articleId),
                                  [this](const ApiResponse &r) {
            if (!r.ok) { notify::warn(this, r.message); return; }
            refreshState();
        });
    } else {
        QJsonObject body;
        body.insert("articleId", m_articleId);
        ApiClient::instance().post("/interaction/favorite", body, [this](const ApiResponse &r) {
            if (!r.ok) { notify::warn(this, r.message); return; }
            refreshState();
        });
    }
}

void ArticleDetailDialog::doLike(int type) {
    QJsonObject body;
    body.insert("targetType", "ARTICLE");
    body.insert("targetId", m_articleId);
    body.insert("type", type);
    ApiClient::instance().post("/interaction/like", body, [this](const ApiResponse &r) {
        if (!r.ok) { notify::warn(this, r.message); return; }
        applyState(r.object()); // /like 返回最新 InteractionStateVO
    });
}

void ArticleDetailDialog::openShare() {
    ShareDialog dlg(m_articleId, this);
    dlg.exec();
}

void ArticleDetailDialog::loadComments() {
    ApiClient::instance().get("/interaction/comment?articleId=" + QString::number(m_articleId),
                              [this](const ApiResponse &r) {
        clearLayout(m_commentList);
        if (!r.ok) {
            m_commentList->addStretch();
            return;
        }
        const QJsonArray roots = r.data.toArray();
        if (roots.isEmpty()) {
            auto *empty = new QLabel(QStringLiteral("暂无评论，来抢沙发～"), this);
            empty->setStyleSheet("color:#9CA3AF;");
            m_commentList->addWidget(empty);
            m_commentList->addStretch();
            return;
        }

        // 顶层 + 缩进一层回复。回复用 QInputDialog 收集，避免每条评论挂常驻输入框。
        std::function<void(const QJsonObject &, int)> renderOne = [&](const QJsonObject &c, int depth) {
            const qint64 id = asLong(c.value("id"));
            auto *block = new QWidget(this);
            auto *col = new QVBoxLayout(block);
            col->setContentsMargins(depth * 24, 0, 0, 0);
            col->setSpacing(2);

            auto *head = new QLabel(QStringLiteral("<b>%1</b>　<span style='color:#9CA3AF;'>%2</span>")
                                        .arg(c.value("userName").toString().toHtmlEscaped(),
                                             c.value("createTime").toString().replace('T', ' ')),
                                    block);
            head->setTextFormat(Qt::RichText);
            auto *body = new QLabel(c.value("content").toString().toHtmlEscaped(), block);
            body->setWordWrap(true);
            body->setStyleSheet("color:#374151;");
            col->addWidget(head);
            col->addWidget(body);

            if (Session::instance().hasPermission("interaction:comment")) {
                auto *reply = new QPushButton(QStringLiteral("回复"), block);
                reply->setObjectName("GhostButton");
                reply->setFixedWidth(56);
                connect(reply, &QPushButton::clicked, this, [this, id]() {
                    bool ok = false;
                    const QString text = QInputDialog::getText(this, QStringLiteral("回复评论"),
                                                               QStringLiteral("回复内容："), QLineEdit::Normal,
                                                               QString(), &ok);
                    if (ok && !text.trimmed().isEmpty()) {
                        postComment(id, text.trimmed());
                    }
                });
                col->addWidget(reply, 0, Qt::AlignLeft);
            }
            m_commentList->addWidget(block);

            for (const QJsonValue &child : c.value("children").toArray()) {
                renderOne(child.toObject(), depth + 1);
            }
        };

        for (const QJsonValue &v : roots) {
            renderOne(v.toObject(), 0);
        }
        m_commentList->addStretch();
    });
}

void ArticleDetailDialog::postComment(qint64 parentId, const QString &content) {
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
