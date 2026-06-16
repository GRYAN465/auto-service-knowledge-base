#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>

class QLabel;
class QPushButton;
class QTableWidget;
class QTreeWidget;
class QTreeWidgetItem;

namespace kb {

/**
 * 知识基础（模块 3）管理页：单页双 Tab。
 *   - 分类 Tab：QTreeWidget 展示层级 + 工具栏（新增根/新增子/编辑/删除/刷新）
 *   - 标签 Tab：表格 + 颜色色块 + QColorDialog 取色
 * 路由名固定为 category（见 V2 菜单种子 303 节点）。按钮按 hasPermission 启停。
 */
class KnowledgeBasePage : public QWidget {
    Q_OBJECT

public:
    explicit KnowledgeBasePage(const QString &title, QWidget *parent = nullptr);

private:
    void buildUi();
    QWidget *buildCategoryTab();
    QWidget *buildTagTab();

    void refreshCategories();
    void refreshTags();

    void addCategory(bool asChild);
    void editCategory();
    void deleteCategory();

    void addTag();
    void editTag();
    void deleteTag();

    void appendCategoryNodes(QTreeWidgetItem *parentItem, const QJsonArray &nodes);
    qint64 selectedCategoryId() const;
    QString selectedCategoryName() const;
    QString categoryNameById(qint64 id) const;
    qint64 selectedTagId() const;

    void setCatStatus(const QString &text, bool error = false);
    void setTagStatus(const QString &text, bool error = false);

    QString m_title;

    QTreeWidget *m_catTree = nullptr;
    QLabel *m_catStatus = nullptr;
    QPushButton *m_catAddRoot = nullptr;
    QPushButton *m_catAddChild = nullptr;
    QPushButton *m_catEdit = nullptr;
    QPushButton *m_catDelete = nullptr;

    QTableWidget *m_tagTable = nullptr;
    QLabel *m_tagStatus = nullptr;
    QPushButton *m_tagAdd = nullptr;
    QPushButton *m_tagEdit = nullptr;
    QPushButton *m_tagDelete = nullptr;
};

} // namespace kb
