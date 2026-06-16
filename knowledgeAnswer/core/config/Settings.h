#pragma once

#include <QSettings>
#include <QString>

namespace kb {

/**
 * 本地配置封装（QSettings）：服务端地址、记住账号等。
 * 单例，应用生命周期内存活。组织名/应用名在 main.cpp 设置。
 */
class Settings {
public:
    static Settings &instance();

    /** REST Base URL，默认 http://localhost:8080/api（对应《API契约.md》§1）。 */
    QString baseUrl() const;
    void setBaseUrl(const QString &url);

    bool rememberUsername() const;
    void setRememberUsername(bool on);

    QString rememberedUsername() const;
    void setRememberedUsername(const QString &username);

private:
    Settings() = default;
    QSettings m_settings; // 默认作用域：注册表/ini，由 org/app 名决定
};

} // namespace kb
