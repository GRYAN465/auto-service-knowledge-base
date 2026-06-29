#pragma once

#include <QHash>
#include <QObject>
#include <QString>

#include <functional>

class QStackedWidget;
class QWidget;

namespace kb {

/**
 * 页面路由：name -> 工厂，懒加载页面到 QStackedWidget 并切换。
 * name 对应 /auth/me 返回的 menus[].name（见《API契约.md》§2）。
 */
class PageRouter : public QObject {
    Q_OBJECT
public:
    using Factory = std::function<QWidget *()>;

    explicit PageRouter(QStackedWidget *stack, QObject *parent = nullptr);

    void registerPage(const QString &name, Factory factory);
    bool hasPage(const QString &name) const;

    /** 切到指定页；首次访问时由工厂创建并加入栈。 */
    void navigate(const QString &name);

    QWidget *currentWidget() const;

signals:
    /** firstVisit=true 表示本次刚懒加载创建，false 表示再次切回已有页面。 */
    void pageShown(const QString &name, bool firstVisit);

private:
    QStackedWidget *m_stack;
    QHash<QString, Factory> m_factories;
    QHash<QString, QWidget *> m_pages;
};

} // namespace kb
