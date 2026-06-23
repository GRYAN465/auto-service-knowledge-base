#include "app/ProfilePage.h"

#include "app/ArticleDetailPanel.h"
#include "app/RefreshablePage.h"
#include "app/UserProfilePanel.h"

#include <QStackedWidget>
#include <QVBoxLayout>

namespace kb {

ProfilePage::ProfilePage(const QString &title, QWidget *parent) : QWidget(parent) {
    Q_UNUSED(title);
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    m_stack = new QStackedWidget(this);
    m_stack->setObjectName("ProfileContentStack");
    m_profile = new UserProfilePanel(0, false, m_stack);
    m_detail = new ArticleDetailPanel(m_stack);
    m_stack->addWidget(m_profile);
    m_stack->addWidget(m_detail);
    root->addWidget(m_stack);

    connect(m_profile, &UserProfilePanel::openArticle, this, [this](qint64 id) {
        m_detail->showArticle(id);
        m_stack->setCurrentWidget(m_detail);
    });
    connect(m_detail, &ArticleDetailPanel::backRequested, this, [this]() {
        m_stack->setCurrentWidget(m_profile);
        m_profile->reload();
    });
    connect(m_detail, &ArticleDetailPanel::authorClicked, this, [this](qint64 authorId) {
        auto *other = new UserProfilePanel(authorId, true, this);
        connect(other, &UserProfilePanel::backRequested, other, &QObject::deleteLater);
        connect(other, &UserProfilePanel::openArticle, this, [this](qint64 id) {
            m_detail->showArticle(id);
            m_stack->setCurrentWidget(m_detail);
        });
        m_stack->addWidget(other);
        m_stack->setCurrentWidget(other);
    });
}

void ProfilePage::refreshPage() {
    if (m_stack && m_stack->currentWidget() == m_profile) {
        m_profile->reload();
    }
}

} // namespace kb
