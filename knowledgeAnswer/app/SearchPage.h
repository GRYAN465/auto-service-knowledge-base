#pragma once

#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace kb {

/**
 * 知识检索页（模块 5，路由 knowledge.search）：
 *   - 顶部筛选栏（关键词 / 分类 / 标签），回车或「检索」触发
 *   - 结果列表（标题 / 分类 / 类型 / 作者 / 浏览 / 更新时间），仅含已上线知识
 *   - 双击行打开只读详情弹窗（ArticleDetailDialog，打开即埋点浏览）
 * 走 GET /search/article；富文本/附件下载在详情弹窗内完成。
 */
class SearchPage : public QWidget {
    Q_OBJECT

public:
    explicit SearchPage(const QString &title, QWidget *parent = nullptr);

private:
    void buildUi();
    void loadFilters();
    void refresh();
    void openDetail();

    qint64 selectedId() const;
    void setStatus(const QString &text, bool error = false);

    QString m_title;
    QLineEdit *m_keyword = nullptr;
    QComboBox *m_categoryFilter = nullptr;
    QComboBox *m_tagFilter = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_status = nullptr;
};

} // namespace kb
