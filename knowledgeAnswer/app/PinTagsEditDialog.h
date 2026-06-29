#pragma once

#include <QDialog>
#include <QJsonArray>

namespace kb {

class PinTagsEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit PinTagsEditDialog(const QJsonArray &allTags, const QVector<qint64> &pinned,
                               QWidget *parent = nullptr);

    QVector<qint64> selectedTagIds() const;

private:
    QVector<qint64> m_selected;
};

} // namespace kb
