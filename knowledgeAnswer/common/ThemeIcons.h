#pragma once

#include <QSize>

class QPushButton;

namespace kb {

/** 主题色 SVG 图标（鼠尾草绿 #9AA69D），供 IconButton 使用。 */
class ThemeIcons {
public:
    enum class Kind {
        Refresh,
        StarOutline,
        StarFilled,
        ThumbUp,
        ThumbUpFilled,
        ThumbDown,
        ThumbDownFilled,
        Share,
        Back,
        Download,
        Upload,
        Trash,
        User,
        Logout,
    };

    static QString path(Kind kind);
    static void applyIconButton(QPushButton *btn, Kind kind, const QString &tooltip, int iconSize = 20);
    static void setIcon(QPushButton *btn, Kind kind, int iconSize = 20);
};

} // namespace kb
