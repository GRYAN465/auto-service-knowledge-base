#pragma once

#include <QStyledItemDelegate>

namespace kb {

/** 侧栏 NavTree：一级分组（不可点）与二级菜单（可点）分色绘制。 */
class NavTreeDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    static constexpr int NavItemKindRole = Qt::UserRole + 2;
    static constexpr int NavIconPathRole = Qt::UserRole + 3;

    explicit NavTreeDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    static bool isNavGroup(const QModelIndex &index);
};

} // namespace kb
