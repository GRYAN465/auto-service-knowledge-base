#include "common/AvatarLabel.h"

namespace kb {

AvatarLabel::AvatarLabel(int sizePx, QWidget *parent)
    : QLabel(parent), m_sizePx(sizePx) {
    setFixedSize(sizePx, sizePx);
    setAlignment(Qt::AlignCenter);
    const int fontSize = qMax(14, sizePx / 2);
    setStyleSheet(QStringLiteral(
                      "background:#9AA69D; color:#FFFFFF; border-radius:%1px; "
                      "font-size:%2px; font-weight:600;")
                      .arg(sizePx / 2)
                      .arg(fontSize));
}

void AvatarLabel::setDisplayName(const QString &name) {
    QString text = name.trimmed();
    if (text.isEmpty()) {
        text = QStringLiteral("?");
    }
    setText(text.left(1).toUpper());
}

} // namespace kb
