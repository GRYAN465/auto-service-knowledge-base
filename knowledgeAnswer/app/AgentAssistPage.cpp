#include "app/AgentAssistPage.h"

#include "app/ArticleDetailDialog.h"
#include "core/auth/Session.h"
#include "core/config/Settings.h"
#include "core/network/ApiClient.h"

#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QWebSocket>

namespace kb {

namespace {

QString wsBaseUrl() {
    // Settings::baseUrl() 形如 http://localhost:8080/api；
    // 只改协议 ws://，保留 /api 路径（WebSocket 端点挂在 context-path /api 下）。
    QString base = Settings::instance().baseUrl();
    if (base.startsWith(QStringLiteral("https://")))
        return QStringLiteral("wss://") + base.mid(8);
    if (base.startsWith(QStringLiteral("http://")))
        return QStringLiteral("ws://") + base.mid(7);
    return base;
}

QString formatDuration(int sec) {
    int m = sec / 60;
    int s = sec % 60;
    return QStringLiteral("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
}

} // namespace

AgentAssistPage::AgentAssistPage(const QString &title, QWidget *parent)
    : QWidget(parent), m_title(title) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 18, 20, 18);
    root->setSpacing(10);

    // 顶部信息栏
    auto *head = new QFrame(this);
    head->setObjectName(QStringLiteral("AgentHead"));
    auto *hl = new QHBoxLayout(head);
    hl->setContentsMargins(16, 12, 16, 12);
    hl->setSpacing(16);
    m_callerLabel = new QLabel(QStringLiteral("主叫：—"), head);
    m_statusLabel = new QLabel(QStringLiteral("状态：空闲"), head);
    m_timerLabel = new QLabel(QStringLiteral("时长：00:00"), head);
    m_toggleBtn = new QPushButton(QStringLiteral("开始会话"), head);
    m_toggleBtn->setObjectName(QStringLiteral("PrimaryButton"));
    hl->addWidget(m_callerLabel);
    hl->addWidget(m_statusLabel);
    hl->addWidget(m_timerLabel);
    hl->addStretch();
    hl->addWidget(m_toggleBtn);
    root->addWidget(head);

    connect(m_toggleBtn, &QPushButton::clicked, this, &AgentAssistPage::onToggleSession);

    // 主体：左转写 / 右推荐
    auto *body = new QHBoxLayout;
    body->setSpacing(12);

    // 转写区
    auto *transcriptHost = new QFrame(this);
    transcriptHost->setObjectName(QStringLiteral("TranscriptArea"));
    auto *tl = new QVBoxLayout(transcriptHost);
    tl->setContentsMargins(12, 12, 12, 12);
    tl->setSpacing(6);
    auto *transcriptTitle = new QLabel(QStringLiteral("实时转写"), transcriptHost);
    transcriptTitle->setObjectName(QStringLiteral("SectionTitle"));
    tl->addWidget(transcriptTitle);
    m_transcriptArea = new QScrollArea(transcriptHost);
    m_transcriptArea->setWidgetResizable(true);
    m_transcriptArea->setObjectName(QStringLiteral("TranscriptScroll"));
    auto *scrollContent = new QWidget(m_transcriptArea);
    m_transcriptLayout = new QVBoxLayout(scrollContent);
    m_transcriptLayout->setContentsMargins(0, 0, 0, 0);
    m_transcriptLayout->setSpacing(8);
    m_transcriptLayout->addStretch();
    m_transcriptArea->setWidget(scrollContent);
    tl->addWidget(m_transcriptArea, 1);
    body->addWidget(transcriptHost, 3);

    // 推荐区
    auto *recHost = new QFrame(this);
    recHost->setObjectName(QStringLiteral("RecommendArea"));
    auto *rl = new QVBoxLayout(recHost);
    rl->setContentsMargins(12, 12, 12, 12);
    rl->setSpacing(8);
    auto *recTitle = new QLabel(QStringLiteral("推荐知识"), recHost);
    recTitle->setObjectName(QStringLiteral("SectionTitle"));
    rl->addWidget(recTitle);
    m_recHint = new QLabel(QStringLiteral("等待通话开始…"), recHost);
    m_recHint->setObjectName(QStringLiteral("MutedLabel"));
    m_recHint->setWordWrap(true);
    rl->addWidget(m_recHint);
    m_recLayout = new QVBoxLayout;
    m_recLayout->setContentsMargins(0, 0, 0, 0);
    m_recLayout->setSpacing(8);
    rl->addLayout(m_recLayout);
    rl->addStretch();
    body->addWidget(recHost, 2);

    root->addLayout(body, 1);

    m_hintLabel = new QLabel(this);
    m_hintLabel->setObjectName(QStringLiteral("MutedLabel"));
    root->addWidget(m_hintLabel);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &AgentAssistPage::tryReconnect);

    m_durationTimer = new QTimer(this);
    m_durationTimer->setInterval(1000);
    connect(m_durationTimer, &QTimer::timeout, this, [this]() {
        m_elapsedSec++;
        m_timerLabel->setText(QStringLiteral("时长：%1").arg(formatDuration(m_elapsedSec)));
    });

    // 进入页面若有遗留活跃会话，提示可恢复
    refreshPage();
}

void AgentAssistPage::refreshPage() {
    // 查询当前坐席是否有 ACTIVE 会话（断线重连/页面重入恢复）
    ApiClient::instance().get(QStringLiteral("/realtime/session/active"),
        [this](const ApiResponse &r) {
            if (r.ok && r.data.isObject()) {
                const auto obj = r.data.toObject();
                const qint64 sid = static_cast<qint64>(obj.value("sessionId").toDouble());
                if (sid > 0) {
                    m_sessionId = sid;
                    m_callerLabel->setText(QStringLiteral("主叫：%1").arg(obj.value("callerNumber").toString()));
                    m_statusLabel->setText(QStringLiteral("状态：通话中"));
                    m_toggleBtn->setText(QStringLiteral("结束会话"));
                    m_elapsedSec = 0;
                    m_durationTimer->start();
                    connectWs(sid);
                    reconnectBackfill(sid);
                }
            }
        });
}

qint64 AgentAssistPage::currentSessionId() const {
    return m_sessionId;
}

void AgentAssistPage::onToggleSession() {
    if (m_sessionId > 0) {
        endSession();
    } else {
        bool ok = false;
        const QString caller = QInputDialog::getText(this, QStringLiteral("开始会话"),
            QStringLiteral("请输入主叫号码："), QLineEdit::Normal, QString(), &ok);
        if (ok)
            startSession(caller.trimmed());
    }
}

void AgentAssistPage::startSession(const QString &callerNumber) {
    QJsonObject body;
    body["callerNumber"] = callerNumber;
    ApiClient::instance().post(QStringLiteral("/realtime/session/start"), body,
        [this, callerNumber](const ApiResponse &r) {
            if (!r.ok) {
                setStatus(r.message.isEmpty() ? QStringLiteral("开始会话失败") : r.message, true);
                return;
            }
            const auto obj = r.data.toObject();
            m_sessionId = static_cast<qint64>(obj.value("sessionId").toDouble());
            m_callerLabel->setText(QStringLiteral("主叫：%1").arg(callerNumber));
            m_statusLabel->setText(QStringLiteral("状态：通话中"));
            m_toggleBtn->setText(QStringLiteral("结束会话"));
            m_elapsedSec = 0;
            m_durationTimer->start();
            m_reconnectAttempts = 0;
            m_ending = false;
            connectWs(m_sessionId);
        });
}

void AgentAssistPage::endSession() {
    if (m_sessionId <= 0)
        return;
    m_ending = true;
    QJsonObject body;
    body["sessionId"] = m_sessionId;
    ApiClient::instance().post(QStringLiteral("/realtime/session/end"), body,
        [this](const ApiResponse &) {
            // 服务器会在 WS 上回推 session_end，由消息处理负责收尾
        });
}

void AgentAssistPage::connectWs(qint64 sessionId) {
    closeWs();
    const QString url = QStringLiteral("%1/ws/realtime/%2?token=%3")
                            .arg(wsBaseUrl())
                            .arg(sessionId)
                            .arg(Session::instance().token());
    m_ws = new QWebSocket(QStringLiteral(""), QWebSocketProtocol::VersionLatest, this);
    connect(m_ws, &QWebSocket::connected, this, &AgentAssistPage::onWsConnected);
    connect(m_ws, &QWebSocket::disconnected, this, &AgentAssistPage::onWsDisconnected);
    connect(m_ws, &QWebSocket::textMessageReceived, this, &AgentAssistPage::onWsTextMessage);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(m_ws, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        setStatus(QStringLiteral("WebSocket 错误：%1").arg(m_ws->errorString()), true);
    });
#else
    connect(m_ws, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, [this](QAbstractSocket::SocketError) {
        setStatus(QStringLiteral("WebSocket 错误：%1").arg(m_ws->errorString()), true);
    });
#endif
    m_ws->open(QUrl(url));
}

void AgentAssistPage::closeWs() {
    if (m_ws) {
        m_ws->disconnect(this);
        m_ws->abort();
        m_ws->deleteLater();
        m_ws = nullptr;
    }
}

void AgentAssistPage::onWsConnected() {
    m_reconnectAttempts = 0;
    setStatus(QString());
    if (m_sessionId > 0)
        reconnectBackfill(m_sessionId);
}

void AgentAssistPage::onWsDisconnected() {
    if (m_ending)
        return; // 主动结束，不重连
    if (m_reconnectAttempts >= 3) {
        setStatus(QStringLiteral("实时连接已断开，请结束会话后重试"), true);
        return;
    }
    const int delay = 1000 << m_reconnectAttempts; // 1s / 2s / 4s
    m_reconnectAttempts++;
    m_reconnectTimer->start(delay);
    setStatus(QStringLiteral("连接断开，%1 后重连…").arg(delay / 1000));
}

void AgentAssistPage::tryReconnect() {
    if (m_sessionId > 0 && !m_ending)
        connectWs(m_sessionId);
}

void AgentAssistPage::reconnectBackfill(qint64 sessionId) {
    ApiClient::instance().get(QStringLiteral("/realtime/transcript?sessionId=%1").arg(sessionId),
        [this](const ApiResponse &r) {
            if (!r.ok)
                return;
            const auto items = r.data.toObject().value("items").toArray();
            // 清空既有转写（保留尾部的 stretch 占位项不删对象，先整体清再补 stretch）
            const int lastIdx = m_transcriptLayout->count() - 1;
            for (int i = lastIdx; i >= 0; --i) {
                auto *it = m_transcriptLayout->takeAt(i);
                delete it->widget();
                delete it;
            }
            m_transcriptLayout->addStretch();
            for (const auto &v : items) {
                const auto m = v.toObject();
                appendTranscript(m.value("speaker").toString(), m.value("content").toString(),
                                 m.value("seqNo").toVariant().toLongLong());
            }
        });
}

void AgentAssistPage::onWsTextMessage(const QString &msg) {
    const auto doc = QJsonDocument::fromJson(msg.toUtf8());
    if (!doc.isObject())
        return;
    const auto root = doc.object();
    const QString type = root.value("type").toString();
    const auto data = root.value("data").toObject();

    if (type == QStringLiteral("transcript")) {
        appendTranscript(data.value("speaker").toString(), data.value("content").toString(),
                         data.value("seqNo").toVariant().toLongLong());
    } else if (type == QStringLiteral("recommendation")) {
        setRecommendations(data.value("articles").toArray(), data.value("triggerText").toString());
    } else if (type == QStringLiteral("session_end")) {
        m_sessionId = 0;
        m_statusLabel->setText(QStringLiteral("状态：已结束"));
        m_toggleBtn->setText(QStringLiteral("开始会话"));
        m_durationTimer->stop();
        closeWs();
        m_recHint->setText(QStringLiteral("会话已结束"));
        m_recHint->setObjectName(QStringLiteral("MutedLabel"));
    }
}

void AgentAssistPage::appendTranscript(const QString &speaker, const QString &content, qint64 seqNo) {
    auto *bubble = new QLabel(content, m_transcriptArea);
    bubble->setWordWrap(true);
    bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bubble->setMaximumWidth(420);
    const bool customer = (speaker == QStringLiteral("customer"));
    bubble->setObjectName(customer ? QStringLiteral("CustomerBubble")
                                   : QStringLiteral("AgentBubble"));
    bubble->setProperty("seqNo", seqNo);

    // 仅客户话术可点：点击即时取该句推荐。装手型光标 + 事件过滤捕获 MouseButtonPress。
    if (customer) {
        bubble->setCursor(Qt::PointingHandCursor);
        bubble->installEventFilter(this);
    }

    auto *row = new QWidget(m_transcriptArea->widget());
    row->setObjectName(QStringLiteral("TranscriptRow"));
    auto *rl = new QHBoxLayout(row);
    rl->setContentsMargins(0, 0, 0, 0);
    if (customer)
        rl->addStretch();
    rl->addWidget(bubble);
    if (!customer)
        rl->addStretch();

    // 在末尾 stretch 之前插入
    const int last = m_transcriptLayout->count() - 1;
    m_transcriptLayout->insertWidget(last < 0 ? 0 : last, row);

    QMetaObject::invokeMethod(m_transcriptArea, [this]() {
        m_transcriptArea->verticalScrollBar()->setValue(
            m_transcriptArea->verticalScrollBar()->maximum());
    }, Qt::QueuedConnection);
}

void AgentAssistPage::setRecommendations(const QJsonArray &articles, const QString &triggerText) {
    // 清空旧推荐
    QLayoutItem *item;
    while ((item = m_recLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    if (articles.isEmpty()) {
        m_recHint->setText(QStringLiteral("暂无匹配知识"));
        m_recHint->setObjectName(QStringLiteral("MutedLabel"));
        return;
    }
    m_recHint->setText(triggerText.isEmpty() ? QString() : QStringLiteral("基于：“%1”").arg(triggerText));

    int rank = 1;
    for (const auto &v : articles) {
        const auto a = v.toObject();
        const qint64 articleId = static_cast<qint64>(a.value("articleId").toDouble());
        const QString title = a.value("title").toString();
        const double score = a.value("score").toDouble();

        auto *card = new QFrame(this);
        card->setObjectName(QStringLiteral("RecommendCard"));
        auto *cl = new QVBoxLayout(card);
        cl->setContentsMargins(10, 8, 10, 8);
        cl->setSpacing(4);

        auto *head = new QLabel(card);
        QString headText = QString("<a href=\"%1\" style=\"color:#6B7F74;text-decoration:none;font-weight:600;\">%2. %3</a>")
                              .arg(articleId).arg(rank++).arg(title.toHtmlEscaped());
        if (score > 0)
            headText += QString("　<span style='color:#757575;'>%1%</span>")
                            .arg(static_cast<int>(score * 100));
        head->setTextFormat(Qt::RichText);
        head->setText(headText);
        head->setWordWrap(true);
        connect(head, &QLabel::linkActivated, this, [this, articleId](const QString &) {
            ArticleDetailDialog dlg(articleId, this);
            dlg.exec();
        });
        cl->addWidget(head);

        auto *actions = new QHBoxLayout;
        actions->setSpacing(8);
        auto *viewBtn = new QPushButton(QStringLiteral("查看"), card);
        auto *copyBtn = new QPushButton(QStringLiteral("复制话术"), card);
        for (auto *b : {viewBtn, copyBtn})
            b->setObjectName(QStringLiteral("GhostButton"));
        connect(viewBtn, &QPushButton::clicked, this, [this, articleId]() {
            ArticleDetailDialog dlg(articleId, this);
            dlg.exec();
        });
        // 复制话术：拉取文章正文（HTML）去标签成纯文本后复制。
        connect(copyBtn, &QPushButton::clicked, this, [this, articleId]() {
            ApiClient::instance().get(
                QStringLiteral("/knowledge/article/%1/view").arg(articleId),
                [this](const ApiResponse &r) {
                    if (!r.ok) {
                        setStatus(QStringLiteral("复制失败：") + r.message, true);
                        return;
                    }
                    const QString html = r.data.toObject().value("content").toString();
                    QString plain = html;
                    plain.remove(QRegularExpression(QStringLiteral("<[^>]+>")));
                    plain.replace(QRegularExpression(QStringLiteral("&nbsp;")), QStringLiteral(" "));
                    plain = plain.simplified();
                    QApplication::clipboard()->setText(plain);
                    setStatus(QStringLiteral("已复制话术到剪贴板"));
                });
        });
        actions->addWidget(viewBtn);
        actions->addWidget(copyBtn);
        actions->addStretch();
        cl->addLayout(actions);

        m_recLayout->addWidget(card);
    }
}

void AgentAssistPage::clearPanel() {
    const int last = m_transcriptLayout->count() - 1;
    while (m_transcriptLayout->count() > 1) {
        auto *it = m_transcriptLayout->takeAt(0);
        delete it->widget();
        delete it;
    }
    QLayoutItem *item;
    while ((item = m_recLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    m_elapsedSec = 0;
    m_timerLabel->setText(QStringLiteral("时长：00:00"));
}

void AgentAssistPage::setStatus(const QString &text, bool error) {
    m_hintLabel->setText(text);
    m_hintLabel->setObjectName(error ? QStringLiteral("ErrorLabel") : QStringLiteral("MutedLabel"));
    m_hintLabel->style()->unpolish(m_hintLabel);
    m_hintLabel->style()->polish(m_hintLabel);
}

bool AgentAssistPage::eventFilter(QObject *watched, QEvent *event) {
    // 客户话术气泡：鼠标按下即触发该句推荐（不区分左右键，单击即可）。
    if (event->type() == QEvent::MouseButtonPress) {
        auto *bubble = qobject_cast<QLabel *>(watched);
        if (bubble && bubble->property("seqNo").isValid() && m_sessionId > 0) {
            const qint64 seqNo = bubble->property("seqNo").toLongLong();
            requestRecommendBySeq(m_sessionId, seqNo);
            return true; // 吞掉事件，避免与 TextSelectableByMouse 的选中文本行为冲突
        }
    }
    return QWidget::eventFilter(watched, event);
}

void AgentAssistPage::requestRecommendBySeq(qint64 sessionId, qint64 seqNo) {
    if (sessionId <= 0)
        return;
    // 过渡反馈：清旧卡片 + 显示加载提示
    QLayoutItem *item;
    while ((item = m_recLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    m_recHint->setText(QStringLiteral("正在检索…"));
    m_recHint->setObjectName(QStringLiteral("MutedLabel"));
    m_recHint->style()->unpolish(m_recHint);
    m_recHint->style()->polish(m_recHint);

    QJsonObject body;
    body["sessionId"] = sessionId;
    body["seqNo"] = seqNo;
    ApiClient::instance().post(QStringLiteral("/realtime/recommend/by-seq"), body,
        [this](const ApiResponse &r) {
            if (!r.ok || !r.data.isObject()) {
                m_recHint->setText(QStringLiteral("检索失败"));
                m_recHint->setObjectName(QStringLiteral("ErrorLabel"));
                m_recHint->style()->unpolish(m_recHint);
                m_recHint->style()->polish(m_recHint);
                setStatus(QStringLiteral("取推荐失败：") + r.message, true);
                return;
            }
            const auto obj = r.data.toObject();
            const auto articles = obj.value("articles").toArray();
            const QString triggerText = obj.value("triggerText").toString();
            setRecommendations(articles, triggerText);
        });
}

} // namespace kb