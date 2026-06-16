#pragma once

#include <QString>
#include <QWidget>

namespace kb {

/**
 * 骨架占位页：居中标题 + 说明 + 路由标签。各业务模块开发时用真实 XxxPage 替换路由注册。
 */
class PlaceholderPage : public QWidget {
    Q_OBJECT
public:
    explicit PlaceholderPage(const QString &title, const QString &routeName,
                             bool phase2 = false, QWidget *parent = nullptr);
};

} // namespace kb
