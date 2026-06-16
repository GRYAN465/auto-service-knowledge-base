#include "app/PlaceholderPage.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace kb {

PlaceholderPage::PlaceholderPage(const QString &title, const QString &routeName,
                                 bool phase2, QWidget *parent)
    : QWidget(parent) {
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(40, 40, 40, 40);
    outer->addStretch();

    auto *titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("PlaceholderTitle");
    titleLabel->setAlignment(Qt::AlignCenter);

    auto *hint = new QLabel(
        phase2 ? QStringLiteral("二期功能 · 骨架占位页，将在对应模块开发时实现")
               : QStringLiteral("骨架占位页，将在对应模块开发时实现"),
        this);
    hint->setObjectName("PlaceholderHint");
    hint->setAlignment(Qt::AlignCenter);

    auto *chips = new QHBoxLayout();
    chips->addStretch();
    auto *routeChip = new QLabel(QStringLiteral("路由  %1").arg(routeName), this);
    routeChip->setObjectName("RouteChip");
    chips->addWidget(routeChip);
    if (phase2) {
        auto *phaseChip = new QLabel(QStringLiteral("二期"), this);
        phaseChip->setObjectName("Phase2Chip");
        chips->addWidget(phaseChip);
    }
    chips->addStretch();

    outer->addWidget(titleLabel);
    outer->addSpacing(10);
    outer->addWidget(hint);
    outer->addSpacing(18);
    outer->addLayout(chips);
    outer->addStretch();
}

} // namespace kb
