#pragma once

#include "app/RefreshablePage.h"

#include <QWidget>

class QLabel;
class QTableWidget;

namespace kb {

/**
 * 我的分享页（模块 6，路由 share）：
 *   - 「收到的分享」Tab（站内通知收件箱）：GET /interaction/share/inbox；双击打开详情并 PUT 标已读后刷新
 *   - 「我发出的」Tab：GET /interaction/share/sent
 *   - 页顶状态行显示未读数（GET /interaction/share/unread-count）
 */
class SharePage : public QWidget, public RefreshablePage {
    Q_OBJECT

public:
    explicit SharePage(const QString &title, QWidget *parent = nullptr);

    void refreshPage() override;

private:
    void buildUi();
    void refresh();
    void loadInbox();
    void loadSent();
    void loadUnread();
    void openInboxDetail();

    QString m_title;
    QLabel *m_status = nullptr;
    QTableWidget *m_inbox = nullptr;
    QTableWidget *m_sent = nullptr;
};

} // namespace kb
