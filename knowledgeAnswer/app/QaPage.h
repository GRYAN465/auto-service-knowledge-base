#pragma once

#include "app/RefreshablePage.h"

#include <QString>
#include <QWidget>

class QJsonArray;
class QJsonObject;
class QLabel;
class QLineEdit;
class QListWidget;
class QMenu;
class QPushButton;
class QScrollArea;
class QToolButton;
class QVBoxLayout;

namespace kb {

/**
 * 智能问答页（模块 9，路由 qa，权限码 ai:qa）：
 *   - 左侧「会话历史」列表（GET /ai/qa/sessions）；点选回放（GET /ai/qa/sessions/{id}/messages）；「新会话」清空
 *   - 主区纵向气泡流（用户问 + 助手答 + 来源角标 + 引用卡片 + 👍/👎）
 *   - 底部输入框 + 发送：POST /ai/qa {question, sessionId?}，记住 sessionId 供追问
 *   - 引用卡点击打开只读详情弹窗（复用 ArticleDetailDialog）；反馈 POST /ai/qa/feedback
 * 异步回调以 QPointer 守卫（LLM 延迟可达数十秒，期间可能切走本页）。
 */
class QaPage : public QWidget, public RefreshablePage {
    Q_OBJECT

public:
    explicit QaPage(const QString &title, QWidget *parent = nullptr);

    void refreshPage() override;

private:
    void buildUi();
    void loadSessions();
    void openSession(qint64 sessionId);
    void startNewSession();
    void send();

    // 气泡构建
    void addUserBubble(const QString &text);
    void addAnswerBubble(const QString &answer, const QString &mode,
                         const QJsonArray &citations, qint64 messageId);
    void clearConversation();
    void scrollToBottom();
    void setStatus(const QString &text, bool error = false);
    void updateKbScopeLabel();

    void sendFeedback(qint64 messageId, bool helpful, QPushButton *up, QPushButton *down);

    QString m_title;
    qint64 m_sessionId = 0;      // 0 = 新会话（首问后由响应回填）

    QListWidget *m_sessionList = nullptr;
    QScrollArea *m_scroll = nullptr;
    QWidget *m_messages = nullptr;
    QVBoxLayout *m_messagesLayout = nullptr;
    QLineEdit *m_input = nullptr;
    QPushButton *m_sendBtn = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_kbScopeLabel = nullptr;
    QMenu *m_kbMenu = nullptr;
    QToolButton *m_kbBtn = nullptr;
    QString m_kbType;
    QString m_kbLabel = QStringLiteral("全部知识库");
};

} // namespace kb
