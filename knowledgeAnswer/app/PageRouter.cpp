#include "app/PageRouter.h"

#include <QStackedWidget>
#include <QWidget>

namespace kb {

PageRouter::PageRouter(QStackedWidget *stack, QObject *parent)
    : QObject(parent), m_stack(stack) {}

void PageRouter::registerPage(const QString &name, Factory factory) {
    m_factories.insert(name, std::move(factory));
}

bool PageRouter::hasPage(const QString &name) const {
    return m_factories.contains(name);
}

void PageRouter::navigate(const QString &name) {
    if (!m_factories.contains(name)) {
        return;
    }
    QWidget *page = m_pages.value(name, nullptr);
    const bool firstVisit = (page == nullptr);
    if (!page) {
        page = m_factories.value(name)();
        m_pages.insert(name, page);
        m_stack->addWidget(page);
    }
    m_stack->setCurrentWidget(page);
    emit pageShown(name, firstVisit);
}

QWidget *PageRouter::currentWidget() const {
    return m_stack ? m_stack->currentWidget() : nullptr;
}

} // namespace kb
