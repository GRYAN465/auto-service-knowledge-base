#include "common/ThemeIcons.h"

#include <QIcon>
#include <QPushButton>
#include <QSize>

namespace kb {

QString ThemeIcons::path(Kind kind) {
    switch (kind) {
    case Kind::Refresh:
        return QStringLiteral(":/icons/refresh.svg");
    case Kind::StarOutline:
        return QStringLiteral(":/icons/star-outline.svg");
    case Kind::StarFilled:
        return QStringLiteral(":/icons/star-filled.svg");
    case Kind::ThumbUp:
        return QStringLiteral(":/icons/thumb-up.svg");
    case Kind::ThumbUpFilled:
        return QStringLiteral(":/icons/thumb-up-filled.svg");
    case Kind::ThumbDown:
        return QStringLiteral(":/icons/thumb-down.svg");
    case Kind::ThumbDownFilled:
        return QStringLiteral(":/icons/thumb-down-filled.svg");
    case Kind::Share:
        return QStringLiteral(":/icons/share.svg");
    case Kind::Back:
        return QStringLiteral(":/icons/back.svg");
    case Kind::Download:
        return QStringLiteral(":/icons/download.svg");
    case Kind::Upload:
        return QStringLiteral(":/icons/upload.svg");
    case Kind::Trash:
        return QStringLiteral(":/icons/trash.svg");
    case Kind::User:
        return QStringLiteral(":/icons/user.svg");
    case Kind::Logout:
        return QStringLiteral(":/icons/logout.svg");
    }
    return {};
}

void ThemeIcons::applyIconButton(QPushButton *btn, Kind kind, const QString &tooltip, int iconSize) {
    if (!btn) {
        return;
    }
    btn->setObjectName(QStringLiteral("IconButton"));
    btn->setFlat(true);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setToolTip(tooltip);
    const int pad = 12;
    btn->setFixedSize(iconSize + pad, iconSize + pad);
    setIcon(btn, kind, iconSize);
}

void ThemeIcons::setIcon(QPushButton *btn, Kind kind, int iconSize) {
    if (!btn) {
        return;
    }
    btn->setIcon(QIcon(path(kind)));
    btn->setIconSize(QSize(iconSize, iconSize));
    btn->setText({});
}

} // namespace kb
