#pragma once

#include "app/RefreshablePage.h"

#include <QWebSocket>
#include <QWidget>

class QLabel;
class QPushButton;
class QScrollArea;
class QVBoxLayout;

namespace kb {

/**
 * 坐席辅助面板（模块 10）：通话中实时展示客户/坐席转写 + 定时 RAG 推荐知识。
 *
 * 会话生命周期：开始会话 → 建立 WebSocket → 接收 transcript/recommendation → 结束会话。
 * WebSocket 端点 /ws/realtime/{sessionId}?token=；推荐复用后端 /ai/qa RAG（坐席无感）。
 * 断线指数退避重连（1s/2s/4s，3 次后停止），重连后拉取 /realtime/transcript 回放。
 */
class AgentAssistPage : public QWidget, public RefreshablePage {
    Q_OBJECT
public:
    explicit AgentAssistPage(const QString &title, QWidget *parent = nullptr);
    void refreshPage() override;

private slots:
    void onToggleSession();
    void onWsConnected();
    void onWsDisconnected();
    void onWsTextMessage(const QString &msg);
    void tryReconnect();

private:
    void startSession(const QString &callerNumber);
    void endSession();
    void connectWs(qint64 sessionId);
    void closeWs();
    void appendTranscript(const QString &speaker, const QString &content);
    void setRecommendations(const QJsonArray &articles, const QString &triggerText);
    void clearPanel();
    void setStatus(const QString &text, bool error = false);
    void reconnectBackfill(qint64 sessionId);
    qint64 currentSessionId() const;

    QString m_title;
    QLabel *m_callerLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_timerLabel = nullptr;
    QPushButton *m_toggleBtn = nullptr;
    QLabel *m_hintLabel = nullptr;

    QScrollArea *m_transcriptArea = nullptr;
    QVBoxLayout *m_transcriptLayout = nullptr;

    QVBoxLayout *m_recLayout = nullptr;
    QLabel *m_recHint = nullptr;

    QWebSocket *m_ws = nullptr;
    qint64 m_sessionId = 0;
    bool m_ending = false;          // 是否由本地主动结束（避免触发重连）
    int m_reconnectAttempts = 0;
    class QTimer *m_reconnectTimer = nullptr;
    class QTimer *m_durationTimer = nullptr;
    int m_elapsedSec = 0;
};

} // namespace kb