#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QTableWidget;

namespace kb {

/**
 * 我的收藏页（模块 6，路由 favorite）：
 *   - GET /interaction/favorite 列出本人收藏（标题 / 分类 / 收藏夹 / 收藏时间）
 *   - 双击行打开只读详情弹窗（ArticleDetailDialog）
 *   - 工具栏「取消收藏」DELETE /interaction/favorite/{articleId} 后刷新
 */
class FavoritePage : public QWidget {
    Q_OBJECT

public:
    explicit FavoritePage(const QString &title, QWidget *parent = nullptr);

private:
    void buildUi();
    void refresh();
    void openDetail();
    void unfavorite();

    qint64 selectedArticleId() const;
    void setStatus(const QString &text, bool error = false);

    QString m_title;
    QTableWidget *m_table = nullptr;
    QPushButton *m_unfavBtn = nullptr;
    QLabel *m_status = nullptr;
};

} // namespace kb
