#include "app/QaPage.h"

#include "app/ArticleDetailDialog.h"
#include "common/ThemeIcons.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QAction>
#include <QActionGroup>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace kb {

namespace {

QString modeText(const QString &mode) {
    if (mode == QStringLiteral("llm")) return QStringLiteral("AI 生成");
    if (mode == QStringLiteral("extractive")) return QStringLiteral("知识摘录");
    if (mode == QStringLiteral("no_hit")) return QStringLiteral("未匹配到知识");
    return mode;
}

} // namespace

QaPage::QaPage(const QString &title, QWidget *parent) : QWidget(parent), m_title(title) {
    buildUi();
    loadSessions();
}

void QaPage::refreshPage() {
    loadSessions();
    if (m_sessionId > 0) {
        openSession(m_sessionId);
    }
}

void QaPage::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName("QaSplitter");
    splitter->setChildrenCollapsible(false);

    // —— 左侧：会话历史 ——
    auto *sideBox = new QWidget(splitter);
    auto *sideLayout = new QVBoxLayout(sideBox);
    sideLayout->setContentsMargins(16, 18, 10, 18);
    sideLayout->setSpacing(10);
    auto *newBtn = new QPushButton(QStringLiteral("＋ 新会话"), sideBox);
    newBtn->setObjectName("PrimaryButton");
    newBtn->setCursor(Qt::PointingHandCursor);
    sideLayout->addWidget(newBtn);
    sideLayout->addWidget(new QLabel(QStringLiteral("历史会话"), sideBox));
    m_sessionList = new QListWidget(sideBox);
    m_sessionList->setObjectName("DataTable");
    sideLayout->addWidget(m_sessionList, 1);
    connect(newBtn, &QPushButton::clicked, this, &QaPage::startNewSession);
    connect(m_sessionList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (item) {
            openSession(item->data(Qt::UserRole).toLongLong());
        }
    });

    // —— 右侧：对话区 ——
    auto *chatBox = new QWidget(splitter);
    auto *chatLayout = new QVBoxLayout(chatBox);
    chatLayout->setContentsMargins(20, 18, 20, 16);
    chatLayout->setSpacing(10);

    m_scroll = new QScrollArea(chatBox);
    m_scroll->setObjectName("QaScroll");
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_messages = new QWidget(m_scroll);
    m_messages->setObjectName("QaCanvas");
    m_messagesLayout = new QVBoxLayout(m_messages);
    m_messagesLayout->setContentsMargins(4, 4, 4, 4);
    m_messagesLayout->setSpacing(12);
    m_messagesLayout->addStretch();
    m_scroll->setWidget(m_messages);
    chatLayout->addWidget(m_scroll, 1);

    m_status = new QLabel(chatBox);
    m_status->setObjectName("StatusLabel");
    chatLayout->addWidget(m_status);

    m_kbScopeLabel = new QLabel(chatBox);
    m_kbScopeLabel->setObjectName("MutedLabel");
    chatLayout->addWidget(m_kbScopeLabel);

    auto *inputBar = new QHBoxLayout();
    m_input = new QLineEdit(chatBox);
    m_input->setPlaceholderText(QStringLiteral("输入你的问题，回车发送"));
    m_input->setClearButtonEnabled(true);
    m_sendBtn = new QPushButton(QStringLiteral("发送"), chatBox);
    m_sendBtn->setObjectName("PrimaryButton");

    m_kbMenu = new QMenu(this);
    m_kbMenu->setObjectName(QStringLiteral("QaKbMenu"));
    auto *kbGroup = new QActionGroup(m_kbMenu);
    kbGroup->setExclusive(true);
    const QVector<QPair<QString, QString>> kbOptions = {
        {QStringLiteral("全部知识库"), QString()},
        {QStringLiteral("客服话术库"), QStringLiteral("SCRIPT")},
        {QStringLiteral("培训学习库"), QStringLiteral("TRAIN")},
        {QStringLiteral("产品知识库"), QStringLiteral("PRODUCT")},
        {QStringLiteral("办公文档库"), QStringLiteral("OFFICE")},
    };
    for (const auto &opt : kbOptions) {
        auto *action = m_kbMenu->addAction(opt.first);
        action->setCheckable(true);
        action->setData(opt.second);
        kbGroup->addAction(action);
        if (opt.second.isEmpty()) {
            action->setChecked(true);
        }
    }
    connect(m_kbMenu, &QMenu::triggered, this, [this](QAction *action) {
        if (!action) {
            return;
        }
        for (QAction *a : m_kbMenu->actions()) {
            a->setChecked(a == action);
        }
        m_kbType = action->data().toString();
        m_kbLabel = action->text();
        updateKbScopeLabel();
    });

    m_kbBtn = new QToolButton(chatBox);
    m_kbBtn->setObjectName(QStringLiteral("IconButton"));
    m_kbBtn->setIcon(QIcon(QStringLiteral(":/icons/chevron-up.svg")));
    m_kbBtn->setIconSize(QSize(20, 20));
    m_kbBtn->setCursor(Qt::PointingHandCursor);
    m_kbBtn->setFixedSize(44, 44);
    connect(m_kbBtn, &QToolButton::clicked, this, [this]() {
        const QPoint topRight = m_kbBtn->mapToGlobal(QPoint(m_kbBtn->width(), 0));
        const QSize menuSize = m_kbMenu->sizeHint();
        m_kbMenu->popup(QPoint(topRight.x() - menuSize.width(), topRight.y() - menuSize.height() - 4));
    });

    inputBar->addWidget(m_input, 1);
    inputBar->addWidget(m_sendBtn);
    inputBar->addWidget(m_kbBtn);
    chatLayout->addLayout(inputBar);
    updateKbScopeLabel();
    connect(m_sendBtn, &QPushButton::clicked, this, &QaPage::send);
    connect(m_input, &QLineEdit::returnPressed, this, &QaPage::send);

    splitter->addWidget(sideBox);
    splitter->addWidget(chatBox);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    sideBox->setMaximumWidth(260);
    sideBox->setMinimumWidth(200);

    root->addWidget(splitter);

    setStatus(QStringLiteral("开始提问吧～ 我会基于已上线知识作答并标注来源。"));
}

void QaPage::loadSessions() {
    QPointer<QaPage> self(this);
    ApiClient::instance().get("/ai/qa/sessions", [self](const ApiResponse &r) {
        if (!self || !r.ok) {
            return;
        }
        self->m_sessionList->clear();
        for (const QJsonValue &v : r.data.toArray()) {
            const QJsonObject o = v.toObject();
            const QString title = o.value("title").toString();
            auto *item = new QListWidgetItem(title.isEmpty() ? QStringLiteral("(未命名会话)") : title,
                                             self->m_sessionList);
            item->setData(Qt::UserRole, o.value("id").toVariant());
        }
    });
}

void QaPage::openSession(qint64 sessionId) {
    if (sessionId <= 0) {
        return;
    }
    m_sessionId = sessionId;
    clearConversation();
    setStatus(QStringLiteral("加载会话历史..."));
    QPointer<QaPage> self(this);
    ApiClient::instance().get("/ai/qa/sessions/" + QString::number(sessionId) + "/messages",
                              [self](const ApiResponse &r) {
        if (!self) {
            return;
        }
        if (!r.ok) {
            self->setStatus(r.message, true);
            return;
        }
        for (const QJsonValue &v : r.data.toArray()) {
            const QJsonObject o = v.toObject();
            self->addUserBubble(o.value("question").toString());
            self->addAnswerBubble(o.value("answer").toString(), o.value("mode").toString(),
                                  o.value("citations").toArray(),
                                  static_cast<qint64>(o.value("id").toDouble()));
        }
        self->setStatus(QStringLiteral("已加载历史会话，可继续追问。"));
        self->scrollToBottom();
    });
}

void QaPage::startNewSession() {
    m_sessionId = 0;
    m_sessionList->clearSelection();
    clearConversation();
    setStatus(QStringLiteral("已开启新会话。"));
    m_input->setFocus();
}

void QaPage::send() {
    const QString question = m_input->text().trimmed();
    if (question.isEmpty()) {
        return;
    }
    addUserBubble(question);
    m_input->clear();
    m_sendBtn->setEnabled(false);
    m_input->setEnabled(false);
    setStatus(QStringLiteral("思考中..."));

    QJsonObject body;
    body["question"] = question;
    if (m_sessionId > 0) {
        body["sessionId"] = m_sessionId;
    }
    const QString kt = m_kbType;
    if (!kt.isEmpty()) {
        body["knowledgeType"] = kt;
    }

    QPointer<QaPage> self(this);
    ApiClient::instance().post("/ai/qa", body, [self](const ApiResponse &r) {
        if (!self) {
            return;
        }
        self->m_sendBtn->setEnabled(true);
        self->m_input->setEnabled(true);
        self->m_input->setFocus();
        if (!r.ok) {
            self->setStatus(r.message, true);
            return;
        }
        const QJsonObject d = r.object();
        const qint64 returnedSession = static_cast<qint64>(d.value("sessionId").toDouble());
        const bool isNew = (self->m_sessionId <= 0);
        if (returnedSession > 0) {
            self->m_sessionId = returnedSession;
        }
        self->addAnswerBubble(d.value("answer").toString(), d.value("mode").toString(),
                              d.value("citations").toArray(),
                              static_cast<qint64>(d.value("messageId").toDouble()));
        self->setStatus(QString());
        if (isNew) {
            self->loadSessions();   // 新会话出现在历史列表
        }
    });
}

void QaPage::addUserBubble(const QString &text) {
    auto *bubble = new QLabel(text, m_messages);
    bubble->setObjectName("ChatUserBubble");
    bubble->setWordWrap(true);
    bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bubble->setMaximumWidth(620);

    auto *row = new QWidget(m_messages);
    auto *rl = new QHBoxLayout(row);
    rl->setContentsMargins(0, 0, 0, 0);
    rl->addStretch();
    rl->addWidget(bubble);
    m_messagesLayout->insertWidget(m_messagesLayout->count() - 1, row);
    scrollToBottom();
}

void QaPage::addAnswerBubble(const QString &answer, const QString &mode,
                             const QJsonArray &citations, qint64 messageId) {
    auto *bubble = new QFrame(m_messages);
    bubble->setObjectName("ChatBotBubble");
    bubble->setMaximumWidth(640);
    auto *bl = new QVBoxLayout(bubble);
    bl->setContentsMargins(14, 12, 14, 12);
    bl->setSpacing(8);

    auto *ans = new QLabel(answer.isEmpty() ? QStringLiteral("（无答案）") : answer, bubble);
    ans->setWordWrap(true);
    ans->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bl->addWidget(ans);

    auto *badge = new QLabel(modeText(mode), bubble);
    badge->setObjectName("ModeBadge");
    badge->setProperty("mode", mode);
    bl->addWidget(badge, 0, Qt::AlignLeft);

    // 引用卡片
    for (const QJsonValue &v : citations) {
        const QJsonObject c = v.toObject();
        const qint64 articleId = static_cast<qint64>(c.value("articleId").toDouble());
        const QString title = c.value("title").toString();
        const double score = c.value("score").toDouble();
        const QString snippet = c.value("snippet").toString();

        auto *card = new QFrame(bubble);
        card->setObjectName("CitationCard");
        auto *cl = new QVBoxLayout(card);
        cl->setContentsMargins(10, 8, 10, 8);
        cl->setSpacing(4);

        QString head = QString("<a href=\"%1\" style=\"color:#6B7F74; text-decoration:none; font-weight:600;\">%2</a>")
                           .arg(articleId)
                           .arg(title.toHtmlEscaped());
        if (score > 0) {
            head += QString("　<span style='color:#757575;'>相关度 %1</span>").arg(score, 0, 'f', 2);
        }
        auto *titleLabel = new QLabel(head, card);
        titleLabel->setTextFormat(Qt::RichText);
        titleLabel->setWordWrap(true);
        connect(titleLabel, &QLabel::linkActivated, this, [this](const QString &href) {
            const qint64 id = href.toLongLong();
            if (id > 0) {
                ArticleDetailDialog dlg(id, this);
                dlg.exec();
            }
        });
        cl->addWidget(titleLabel);

        if (!snippet.trimmed().isEmpty()) {
            auto *snip = new QLabel(snippet.trimmed(), card);
            snip->setObjectName("CitationSnippet");
            snip->setWordWrap(true);
            cl->addWidget(snip);
        }
        bl->addWidget(card);
    }

    // 反馈（落库消息才可反馈）
    if (messageId > 0) {
        auto *fbRow = new QWidget(bubble);
        fbRow->setAutoFillBackground(false);
        fbRow->setAttribute(Qt::WA_TranslucentBackground);
        auto *fl = new QHBoxLayout(fbRow);
        fl->setContentsMargins(0, 0, 0, 0);
        fl->setSpacing(6);
        auto *up = new QPushButton(fbRow);
        auto *down = new QPushButton(fbRow);
        ThemeIcons::applyIconButton(up, ThemeIcons::Kind::ThumbUp, QStringLiteral("回答有用"));
        ThemeIcons::applyIconButton(down, ThemeIcons::Kind::ThumbDown, QStringLiteral("回答没用"));
        fl->addWidget(up);
        fl->addWidget(down);
        fl->addStretch();
        connect(up, &QPushButton::clicked, this, [this, messageId, up, down]() {
            sendFeedback(messageId, true, up, down);
        });
        connect(down, &QPushButton::clicked, this, [this, messageId, up, down]() {
            sendFeedback(messageId, false, up, down);
        });
        bl->addWidget(fbRow);
    }

    auto *row = new QWidget(m_messages);
    auto *rl = new QHBoxLayout(row);
    rl->setContentsMargins(0, 0, 0, 0);
    rl->addWidget(bubble);
    rl->addStretch();
    m_messagesLayout->insertWidget(m_messagesLayout->count() - 1, row);
    scrollToBottom();
}

void QaPage::sendFeedback(qint64 messageId, bool helpful, QPushButton *up, QPushButton *down) {
    QJsonObject body;
    body["messageId"] = messageId;
    body["helpful"] = helpful ? 1 : 0;   // 后端 FeedbackRequest.helpful 为 Integer(1赞/0踩)
    QPointer<QaPage> self(this);
    QPointer<QPushButton> upP(up), downP(down);
    ApiClient::instance().post("/ai/qa/feedback", body, [self, upP, downP, helpful](const ApiResponse &r) {
        if (!self) {
            return;
        }
        if (!r.ok) {
            self->setStatus(r.message, true);
            return;
        }
        if (upP) {
            upP->setEnabled(false);
            if (helpful) {
                ThemeIcons::setIcon(upP, ThemeIcons::Kind::ThumbUpFilled);
                upP->setToolTip(QStringLiteral("已反馈：有用"));
            }
        }
        if (downP) {
            downP->setEnabled(false);
            if (!helpful) {
                ThemeIcons::setIcon(downP, ThemeIcons::Kind::ThumbDownFilled);
                downP->setToolTip(QStringLiteral("已反馈：没用"));
            }
        }
        self->setStatus(QStringLiteral("感谢反馈！"));
    });
}

void QaPage::clearConversation() {
    // 删除除尾部 stretch 外的所有行
    while (m_messagesLayout->count() > 1) {
        QLayoutItem *item = m_messagesLayout->takeAt(0);
        if (QWidget *w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
}

void QaPage::scrollToBottom() {
    const auto doScroll = [this]() {
        if (m_messages) {
            m_messages->adjustSize();
        }
        if (!m_scroll) {
            return;
        }
        if (QScrollBar *bar = m_scroll->verticalScrollBar()) {
            bar->setValue(bar->maximum());
        }
    };
    doScroll();
    for (int delayMs : {0, 50, 120, 250}) {
        QTimer::singleShot(delayMs, this, doScroll);
    }
}

void QaPage::updateKbScopeLabel() {
    if (!m_kbScopeLabel) {
        return;
    }
    m_kbScopeLabel->setText(QStringLiteral("检索范围：%1").arg(m_kbLabel));
    if (m_kbBtn) {
        m_kbBtn->setToolTip(QStringLiteral("选择知识库（当前：%1）").arg(m_kbLabel));
    }
}

void QaPage::setStatus(const QString &text, bool error) {
    m_status->setText(text);
    m_status->setStyleSheet(error ? "color:#B94A48;" : "color:#757575;");
    if (error && !text.isEmpty()) {
        notify::warn(this, text);
    }
}

} // namespace kb
