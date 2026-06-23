#include "app/PinTagsEditDialog.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace kb {

PinTagsEditDialog::PinTagsEditDialog(const QJsonArray &allTags, const QVector<qint64> &pinned,
                                     QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("编辑常用标签"));
    resize(480, 420);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *host = new QWidget(scroll);
    auto *list = new QVBoxLayout(host);
    list->setSpacing(8);

    for (const QJsonValue &v : allTags) {
        const QJsonObject t = v.toObject();
        const qint64 id = static_cast<qint64>(t.value("id").toDouble());
        auto *box = new QCheckBox(t.value("name").toString(), host);
        box->setProperty("tagId", id);
        if (pinned.contains(id)) {
            box->setChecked(true);
        }
        list->addWidget(box);
    }
    list->addStretch();
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
        for (QCheckBox *box : host->findChildren<QCheckBox *>()) {
            if (box->isChecked()) {
                m_selected.append(box->property("tagId").toLongLong());
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
