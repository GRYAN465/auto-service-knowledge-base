#include "app/PinTagsEditDialog.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace kb {

namespace {

constexpr int kGridColumns = 5;

} // namespace

PinTagsEditDialog::PinTagsEditDialog(const QJsonArray &allTags, const QVector<qint64> &pinned,
                                     QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("编辑常用标签"));
    resize(520, 420);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *host = new QWidget(scroll);
    auto *grid = new QGridLayout(host);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(10);

    int row = 0;
    int col = 0;
    for (const QJsonValue &v : allTags) {
        const QJsonObject t = v.toObject();
        const qint64 id = static_cast<qint64>(t.value("id").toDouble());
        auto *chip = new QPushButton(t.value("name").toString(), host);
        chip->setObjectName("TagPickerChip");
        chip->setCheckable(true);
        chip->setChecked(pinned.contains(id));
        chip->setCursor(Qt::PointingHandCursor);
        chip->setProperty("tagId", id);
        chip->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        grid->addWidget(chip, row, col);
        if (++col >= kGridColumns) {
            col = 0;
            ++row;
        }
    }
    scroll->setWidget(host);
    root->addWidget(scroll, 1);

    auto *footer = new QHBoxLayout();
    footer->addStretch();
    auto *cancel = new QPushButton(QStringLiteral("取消"), this);
    cancel->setObjectName("GhostButton");
    auto *ok = new QPushButton(QStringLiteral("保存"), this);
    ok->setObjectName("PrimaryButton");
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(ok, &QPushButton::clicked, this, [this, host]() {
        m_selected.clear();
        for (QPushButton *chip : host->findChildren<QPushButton *>()) {
            if (chip->objectName() == QStringLiteral("TagPickerChip") && chip->isChecked()) {
                m_selected.append(chip->property("tagId").toLongLong());
            }
        }
        accept();
    });
    footer->addWidget(cancel);
    footer->addWidget(ok);
    root->addLayout(footer);
}

QVector<qint64> PinTagsEditDialog::selectedTagIds() const {
    return m_selected;
}

} // namespace kb
