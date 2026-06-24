#include "core/config/Settings.h"

namespace kb {

namespace {
constexpr auto kBaseUrl = "net/baseUrl";
constexpr auto kRememberUser = "login/rememberUsername";
constexpr auto kRememberedName = "login/rememberedUsername";
constexpr auto kDefaultBaseUrl = "http://localhost:8080/api";
} // namespace

Settings &Settings::instance() {
    static Settings *inst = new Settings();
    return *inst;
}

QString Settings::normalizeBaseUrl(const QString &url) {
    QString u = url.trimmed();
    while (u.endsWith(QLatin1Char('/'))) {
        u.chop(1);
    }
    if (u.isEmpty()) {
        return QString::fromLatin1(kDefaultBaseUrl);
    }
    if (!u.endsWith(QStringLiteral("/api"), Qt::CaseInsensitive)) {
        u += QStringLiteral("/api");
    }
    return u;
}

QString Settings::baseUrl() const {
    return normalizeBaseUrl(m_settings.value(kBaseUrl, kDefaultBaseUrl).toString());
}

void Settings::setBaseUrl(const QString &url) {
    m_settings.setValue(kBaseUrl, normalizeBaseUrl(url));
}

bool Settings::rememberUsername() const {
    return m_settings.value(kRememberUser, true).toBool();
}

void Settings::setRememberUsername(bool on) {
    m_settings.setValue(kRememberUser, on);
}

QString Settings::rememberedUsername() const {
    return m_settings.value(kRememberedName).toString();
}

void Settings::setRememberedUsername(const QString &username) {
    m_settings.setValue(kRememberedName, username);
}

} // namespace kb
