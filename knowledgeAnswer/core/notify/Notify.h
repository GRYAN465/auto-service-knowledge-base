#pragma once

#include <QMessageBox>
#include <QString>
#include <QWidget>

/**
 * 全局轻量提示。骨架阶段用 QMessageBox 实现；后续可替换为非阻塞 toast。
 */
namespace kb::notify {

inline void info(QWidget *parent, const QString &msg) {
    QMessageBox::information(parent, QStringLiteral("提示"), msg);
}

inline void warn(QWidget *parent, const QString &msg) {
    QMessageBox::warning(parent, QStringLiteral("注意"), msg);
}

inline void error(QWidget *parent, const QString &msg) {
    QMessageBox::critical(parent, QStringLiteral("错误"), msg);
}

} // namespace kb::notify
