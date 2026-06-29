#include "common/NavTreeDelegate.h"

#include <QIcon>
#include <QPainter>
#include <QTreeView>
#include <QWidget>

namespace kb {

namespace {

constexpr int kLinkRowHeight = 38;
constexpr int kGroupRowHeight = 28;
constexpr int kGroupSectionGap = 10;
constexpr int kNavIconSize = 18;
constexpr int kNavIconGap = 8;

QColor linkTextColor(bool selected) {
    Q_UNUSED(selected);
    return QColor("#000000");
}

QColor navFillColor(bool selected, bool hovered) {
    if (selected) {
        return QColor(154, 166, 157, 48);
    }
    if (hovered) {
        return QColor(254, 254, 254, 72);
    }
    return Qt::transparent;
}

} // namespace

NavTreeDelegate::NavTreeDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

bool NavTreeDelegate::isNavGroup(const QModelIndex &index) {
    return index.data(NavItemKindRole).toString() == QStringLiteral("group");
}

void NavTreeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const {
    if (!index.isValid()) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    const bool group = isNavGroup(index);
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;

    QRect rowRect = option.rect;
    if (group && index.row() > 0) {
        rowRect.adjust(0, kGroupSectionGap / 2, 0, 0);
    }

    const QString text = index.data(Qt::DisplayRole).toString();

    if (group) {
        QFont font = option.font;
        font.setPixelSize(12);
        font.setWeight(QFont::DemiBold);
        painter->setFont(font);
        painter->setPen(QColor("#757575"));
        const QRect textRect = rowRect.adjusted(10, 0, -8, 0);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
        painter->restore();
        return;
    }

    // 高亮底：铺满侧栏可用宽度（option.rect 因树形缩进偏窄，不能直接用）
    QRect bgRect = option.rect.adjusted(0, 1, 0, -1);
    if (const QWidget *w = option.widget) {
        bgRect.setLeft(4);
        bgRect.setRight(w->width() - 4);
    }

    const QColor fill = navFillColor(selected, hovered);
    if (fill.alpha() > 0) {
        painter->setBrush(fill);
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(bgRect, 8, 8);
    }

    QFont font = option.font;
    font.setPixelSize(14);
    font.setWeight(selected ? QFont::DemiBold : QFont::Normal);
    painter->setFont(font);
    painter->setPen(linkTextColor(selected));

    const int contentPad = index.parent().isValid() ? 8 : 10;
    const int contentLeft = option.rect.left() + contentPad;
    int textLeft = contentLeft;
    const QString iconPath = index.data(NavIconPathRole).toString();
    if (!iconPath.isEmpty()) {
        const QIcon icon(iconPath);
        const int iconY = option.rect.center().y() - kNavIconSize / 2;
        icon.paint(painter, contentLeft, iconY, kNavIconSize, kNavIconSize);
        textLeft = contentLeft + kNavIconSize + kNavIconGap;
    }

    const QRect textRect(option.rect.left(), option.rect.top(),
                         option.rect.width() - contentPad, option.rect.height());
    painter->drawText(textRect.adjusted(textLeft - option.rect.left(), 0, -8, 0),
                      Qt::AlignLeft | Qt::AlignVCenter, text);

    painter->restore();
}

QSize NavTreeDelegate::sizeHint(const QStyleOptionViewItem &option,
                                const QModelIndex &index) const {
    if (!index.isValid()) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    int height = isNavGroup(index) ? kGroupRowHeight : kLinkRowHeight;
    if (isNavGroup(index) && index.row() > 0) {
        height += kGroupSectionGap;
    }
    return {option.rect.width(), height};
}

} // namespace kb
