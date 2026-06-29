#pragma once

#include "app/RefreshablePage.h"

#include <QJsonObject>
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace kb {

/**
 * 知识管理后台（模块 4，路由 knowledge.manage）：
 *   - 顶部筛选栏（关键词 / 分类 / 状态 / 类型）
 *   - 列表（标题 / 分类 / 类型 / 状态 / 作者 / 浏览 / 更新时间）
 *   - 工具栏（新增 / 编辑 / 删除 / 提交审核 / 上线 / 下线），按权限码启停
 * 编辑走 ArticleEditorDialog；状态机动作调对应 POST 后刷新。
 */
class ArticleManagePage : public QWidget, public RefreshablePage {
    Q_OBJECT

public:
    explicit ArticleManagePage(const QString &title, QWidget *parent = nullptr);

    void refreshPage() override;

private:
    void buildUi();
    void loadCategoryFilter();
    void refresh();

    void createArticle();
    void editArticle();
    void deleteArticle();
    void submitArticle();
    void onlineArticle();
    void offlineArticle();

    qint64 selectedId() const;
    QString selectedStatus() const;
    QString selectedTitle() const;
    void doAction(const QString &subPath, const QJsonObject &body, const QString &okText);
    void setStatus(const QString &text, bool error = false);

    QString m_title;
    QLineEdit *m_keyword = nullptr;
    QComboBox *m_categoryFilter = nullptr;
    QComboBox *m_statusFilter = nullptr;
    QComboBox *m_typeFilter = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_status = nullptr;

    QPushButton *m_create = nullptr;
    QPushButton *m_edit = nullptr;
    QPushButton *m_delete = nullptr;
    QPushButton *m_submit = nullptr;
    QPushButton *m_online = nullptr;
    QPushButton *m_offline = nullptr;
};

} // namespace kb
