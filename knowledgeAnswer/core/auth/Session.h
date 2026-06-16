#pragma once

#include <QJsonObject>
#include <QSet>
#include <QString>
#include <QVector>

namespace kb {

/** 导航菜单节点；name 对应客户端 PageRouter 注册名（见《API契约.md》§2）。 */
struct MenuItem {
    QString name;
    QString title;
    QString icon;
    QVector<MenuItem> children;
};

/** 登录用户。 */
struct CurrentUser {
    qint64 id = 0;
    QString username;
    QString realName;
    qint64 orgId = 0;
};

/**
 * 会话：保存 token、当前用户、角色、权限码集合、菜单树。
 * 供导航构建（菜单）与按钮级鉴权（hasPermission）使用。单例。
 */
class Session {
public:
    static Session &instance();

    void setToken(const QString &token) { m_token = token; }
    QString token() const { return m_token; }
    bool isLoggedIn() const { return !m_token.isEmpty(); }

    /** 用 GET /auth/me 的 data 对象填充用户/角色/权限/菜单。 */
    void applyMe(const QJsonObject &data);

    const CurrentUser &user() const { return m_user; }
    const QVector<QString> &roles() const { return m_roles; }
    const QVector<MenuItem> &menus() const { return m_menus; }

    /** 按钮级权限判断（权限码精确匹配）。 */
    bool hasPermission(const QString &code) const { return m_permissions.contains(code); }
    int permissionCount() const { return m_permissions.size(); }

    void clear();

private:
    Session() = default;

    QString m_token;
    CurrentUser m_user;
    QVector<QString> m_roles;
    QSet<QString> m_permissions;
    QVector<MenuItem> m_menus;
};

} // namespace kb
