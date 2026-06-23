#include "common/NavTreeDelegate.h"

#include <QPainter>
#include <QTreeView>

namespace kb {

namespace {

constexpr int kLinkRowHeight = 38;
constexpr int kGroupRowHeight = 28;
constexpr int kGroupSectionGap = 10;

QColor linkTextColor(bool selected) {
    return selected ? QColor("#2563EB") : QColor("#374151");
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

    QRect rect = option.rect.adjusted(4, 1, -4, -1);
    if (group && index.row() > 0) {
        rect.adjust(0, kGroupSectionGap / 2, 0, 0);
    }

    const QString text = index.data(Qt::DisplayRole).toString();

    if (group) {
        QFont font = option.font;
        font.setPixelSize(12);
        font.setWeight(QFont::DemiBold);
        painter->setFont(font);
        painter->setPen(QColor("#94A3B8"));
        const QRect textRect = rect.adjusted(10, 0, -8, 0);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
        painter->restore();
        return;
    }

    if (selected) {
        painter->setBrush(QColor("#E8F0FE"));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(rect, 8, 8);
    } else if (hovered) {
        painter->setBrush(QColor("#F0F3F8"));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(rect, 8, 8);
    }

    QFont font = option.font;
    font.setPixelSize(14);
    font.setWeight(selected ? QFont::DemiBold : QFont::Normal);
    painter->setFont(font);
    painter->setPen(linkTextColor(selected));

    const int leftPad = index.parent().isValid() ? 22 : 12;
    const QRect textRect = rect.adjusted(leftPad, 0, -8, 0);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);

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
