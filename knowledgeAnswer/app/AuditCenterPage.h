#pragma once

#include <QJsonObject>
#include <QWidget>

class QLabel;
class QPushButton;
class QTableWidget;
class QTextEdit;

namespace kb {

/**
 * 审核中心（模块 4，路由 audit）：列出待审核（PENDING_AUDIT）知识，
 * 选中后右侧预览正文，可「通过 / 驳回（填意见）」。需 knowledge:article:audit。
 */
class AuditCenterPage : public QWidget {
    Q_OBJECT

public:
    explicit AuditCenterPage(const QString &title, QWidget *parent = nullptr);

private:
    void buildUi();
    void refresh();
    void previewSelected();
    void pass();
    void reject();

    qint64 selectedId() const;
    QString selectedTitle() const;
    void doAudit(const QJsonObject &body, const QString &okText);
    void setStatus(const QString &text, bool error = false);

    QString m_title;
    QTableWidget *m_table = nullptr;
    QTextEdit *m_preview = nullptr;
    QLabel *m_status = nullptr;
    QPushButton *m_pass = nullptr;
    QPushButton *m_reject = nullptr;
};

} // namespace kb
