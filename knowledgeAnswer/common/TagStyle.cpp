#include "common/TagStyle.h"

#include <QColor>
#include <QLabel>

namespace kb {
namespace TagStyle {

QLabel *createTagNameChip(const QString &name, const QString &color, QWidget *parent) {
    auto *chip = new QLabel(name, parent);
    chip->setObjectName("TagNameChip");
    QString c = color.trimmed();
    if (c.isEmpty()) {
        c = QStringLiteral("#757575");
    }
    QColor base(c);
    const QString bg = base.isValid()
                           ? QStringLiteral("rgba(%1,%2,%3,0.38)")
                                 .arg(base.red())
                                 .arg(base.green())
                                 .arg(base.blue())
                           : QStringLiteral("#EEF2F7");
    const QString fg = base.isValid() ? base.name(QColor::HexRgb) : QStringLiteral("#4A4A4A");
    chip->setStyleSheet(QStringLiteral(
                            "background:%1; color:%2; border-radius:6px; padding:3px 10px; font-size:12px;")
                            .arg(bg, fg));
    return chip;
}

} // namespace TagStyle
} // namespace kb
