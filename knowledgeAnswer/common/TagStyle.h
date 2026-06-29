#pragma once

#include <QString>

class QLabel;
class QWidget;

namespace kb {
namespace TagStyle {

/** 标签名 chip：浅色底 + 同色字（38% 透明度背景）。 */
QLabel *createTagNameChip(const QString &name, const QString &color, QWidget *parent);

} // namespace TagStyle
} // namespace kb
