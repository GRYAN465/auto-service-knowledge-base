#pragma once

#include <QLoggingCategory>
#include <QString>

namespace kb::log {

inline const QLoggingCategory &cat() {
    static const QLoggingCategory c("kb");
    return c;
}

inline void info(const QString &msg) { qCInfo(cat()).noquote() << msg; }
inline void warn(const QString &msg) { qCWarning(cat()).noquote() << msg; }
inline void error(const QString &msg) { qCCritical(cat()).noquote() << msg; }

} // namespace kb::log
