#include "core/config/PinnedTagsStore.h"

#include <QSettings>
#include <QStringList>

namespace kb {

PinnedTagsStore &PinnedTagsStore::instance() {
    static PinnedTagsStore s;
    return s;
}

QString PinnedTagsStore::key(qint64 userId) const {
    return QStringLiteral("pinnedTags/user_%1").arg(userId);
}

QVector<qint64> PinnedTagsStore::pinnedTagIds(qint64 userId) const {
    QSettings settings;
    const QStringList parts = settings.value(key(userId)).toStringList();
    QVector<qint64> ids;
    ids.reserve(parts.size());
    for (const QString &p : parts) {
        bool ok = false;
        const qint64 id = p.toLongLong(&ok);
        if (ok && id > 0) {
            ids.append(id);
        }
    }
    return ids;
}

void PinnedTagsStore::setPinnedTagIds(qint64 userId, const QVector<qint64> &tagIds) {
    QStringList parts;
    for (qint64 id : tagIds) {
        parts << QString::number(id);
    }
    QSettings settings;
    settings.setValue(key(userId), parts);
}

} // namespace kb
