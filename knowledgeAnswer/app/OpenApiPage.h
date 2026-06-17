#pragma once

#include <QJsonObject>
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace kb {

/**
 * 开放 API 管理页（模块 8，路由 openapi）：
 *   - 管理开放应用 AppKey/AppSecret、启停、限流和 scope
 *   - 对接 /openapi/app，按钮权限使用 openapi:app:*
 */
class OpenApiPage : public QWidget {
    Q_OBJECT

public:
    explicit OpenApiPage(const QString &title, QWidget *parent = nullptr);

private:
    void buildUi();
    void refresh();
    void createApp();
    void editApp();
    void deleteApp();
    void resetSecret();
    void fillTable(const QJsonObject &pageData);
    QString buildListPath() const;
    qint64 selectedId() const;
    QString selectedName() const;
    void setStatus(const QString &text, bool error = false);

    QString m_title;
    QLineEdit *m_keyword = nullptr;
    QComboBox *m_statusFilter = nullptr;
    QLabel *m_status = nullptr;
    QTableWidget *m_table = nullptr;
    QPushButton *m_add = nullptr;
    QPushButton *m_edit = nullptr;
    QPushButton *m_delete = nullptr;
    QPushButton *m_resetSecret = nullptr;
};

} // namespace kb
