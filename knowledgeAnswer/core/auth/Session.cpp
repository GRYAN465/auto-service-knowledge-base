#include "core/auth/Session.h"

#include <QJsonArray>
#include <QJsonValue>

namespace kb {

namespace {
MenuItem parseMenu(const QJsonObject &o) {
    MenuItem m;
    m.name = o.value("name").toString();
    m.title = o.value("title").toString();
    m.icon = o.value("icon").toString();
    const QJsonArray children = o.value("children").toArray();
    for (const QJsonValue &c : children) {
        if (c.isObject()) {
            m.children.append(parseMenu(c.toObject()));
        }
    }
    return m;
}
} // namespace

Session &Session::instance() {
    static Session *inst = new Session();
    return *inst;
}

void Session::applyMe(const QJsonObject &data) {
    const QJsonObject u = data.value("user").toObject();
    m_user.id = static_cast<qint64>(u.value("id").toDouble());
    m_user.username = u.value("username").toString();
    m_user.realName = u.value("realName").toString();
    m_user.orgId = static_cast<qint64>(u.value("orgId").toDouble());

    m_roles.clear();
    for (const QJsonValue &r : data.value("roles").toArray()) {
        m_roles.append(r.toString());
    }

    m_permissions.clear();
    for (const QJsonValue &p : data.value("permissions").toArray()) {
        m_permissions.insert(p.toString());
    }

    m_menus.clear();
    for (const QJsonValue &m : data.value("menus").toArray()) {
        if (m.isObject()) {
            m_menus.append(parseMenu(m.toObject()));
        }
    }
}

void Session::clear() {
    m_token.clear();
    m_user = CurrentUser{};
    m_roles.clear();
    m_permissions.clear();
    m_menus.clear();
}

} // namespace kb
