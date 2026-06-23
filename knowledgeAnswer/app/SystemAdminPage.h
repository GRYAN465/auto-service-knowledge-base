#pragma once

#include "app/RefreshablePage.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>

#include <functional>

class QLabel;
class QPushButton;
class QTableWidget;
class QTabWidget;

namespace kb {

class SystemAdminPage : public QWidget, public RefreshablePage {
    Q_OBJECT

public:
    explicit SystemAdminPage(const QString &title, const QString &routeName, QWidget *parent = nullptr);

    void refreshPage() override;

private:
    void buildUi();
    void refresh();
    void refreshUsers();
    void refreshRoles();
    void refreshPermissions();
    void refreshLogs();
    void fillPageTable(const QJsonObject &pageData, const QStringList &headers,
                       const std::function<QStringList(const QJsonObject &)> &rowMapper);
    void fillPermissionRows(const QJsonArray &items, int depth);
    void fillLogTable(QTableWidget *table, const QJsonObject &pageData, const QStringList &headers,
                      const std::function<QStringList(const QJsonObject &)> &rowMapper);
    void createItem();
    void editItem();
    void deleteItem();
    void toggleUserStatus();
    void resetUserPassword();
    void assignRolePermissions();
    qint64 selectedId() const;
    void setStatus(const QString &text, bool error = false);

    QString m_title;
    QString m_routeName;
    QLabel *m_status = nullptr;
    QTableWidget *m_table = nullptr;
    QTabWidget *m_logTabs = nullptr;
    QTableWidget *m_loginLogTable = nullptr;
    QTableWidget *m_operationLogTable = nullptr;
    QPushButton *m_add = nullptr;
    QPushButton *m_edit = nullptr;
    QPushButton *m_delete = nullptr;
    QPushButton *m_toggleStatus = nullptr;
    QPushButton *m_resetPassword = nullptr;
    QPushButton *m_assignPermissions = nullptr;
};

} // namespace kb
