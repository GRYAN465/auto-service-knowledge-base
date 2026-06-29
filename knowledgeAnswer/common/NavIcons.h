#pragma once

#include <QString>

namespace kb {

/** 侧栏导航菜单 route → SVG 图标路径。 */
class NavIcons {
public:
    static QString pathForRoute(const QString &routeName);
};

} // namespace kb
