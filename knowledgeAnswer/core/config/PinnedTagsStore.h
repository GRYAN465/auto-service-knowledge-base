#pragma once

#include <QVector>

namespace kb {

/** 常用标签 pin 列表，按 userId 存 QSettings。 */
class PinnedTagsStore {
public:
    static PinnedTagsStore &instance();

    QVector<qint64> pinnedTagIds(qint64 userId) const;
    void setPinnedTagIds(qint64 userId, const QVector<qint64> &tagIds);

private:
    PinnedTagsStore() = default;
    QString key(qint64 userId) const;
};

} // namespace kb
