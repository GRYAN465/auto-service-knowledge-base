#include "common/NavIcons.h"

namespace kb {

QString NavIcons::pathForRoute(const QString &routeName) {
    if (routeName == QStringLiteral("dashboard")) {
        return QStringLiteral(":/icons/nav-home.svg");
    }
    if (routeName == QStringLiteral("knowledge.search")) {
        return QStringLiteral(":/icons/nav-search.svg");
    }
    if (routeName == QStringLiteral("share")) {
        return QStringLiteral(":/icons/share.svg");
    }
    if (routeName == QStringLiteral("personal.center")) {
        return QStringLiteral(":/icons/user.svg");
    }
    if (routeName == QStringLiteral("knowledge.manage")) {
        return QStringLiteral(":/icons/nav-book.svg");
    }
    if (routeName == QStringLiteral("audit")) {
        return QStringLiteral(":/icons/nav-audit.svg");
    }
    if (routeName == QStringLiteral("category")) {
        return QStringLiteral(":/icons/nav-tags.svg");
    }
    if (routeName == QStringLiteral("statistics")) {
        return QStringLiteral(":/icons/nav-chart.svg");
    }
    if (routeName == QStringLiteral("openapi")) {
        return QStringLiteral(":/icons/nav-api.svg");
    }
    if (routeName == QStringLiteral("qa")) {
        return QStringLiteral(":/icons/nav-robot.svg");
    }
    if (routeName == QStringLiteral("agent")) {
        return QStringLiteral(":/icons/nav-headset.svg");
    }
    if (routeName == QStringLiteral("cc.config")) {
        return QStringLiteral(":/icons/nav-phone.svg");
    }
    if (routeName == QStringLiteral("ai.config")) {
        return QStringLiteral(":/icons/nav-settings.svg");
    }
    if (routeName == QStringLiteral("system.user")) {
        return QStringLiteral(":/icons/nav-team.svg");
    }
    if (routeName == QStringLiteral("system.role")) {
        return QStringLiteral(":/icons/nav-role.svg");
    }
    if (routeName == QStringLiteral("system.permission")) {
        return QStringLiteral(":/icons/nav-lock.svg");
    }
    if (routeName == QStringLiteral("system.log")) {
        return QStringLiteral(":/icons/nav-log.svg");
    }
    return {};
}

} // namespace kb
