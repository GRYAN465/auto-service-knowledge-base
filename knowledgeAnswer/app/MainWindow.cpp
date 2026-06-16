#include "app/MainWindow.h"

#include "app/KnowledgeBasePage.h"
#include "app/PageRouter.h"
#include "app/PlaceholderPage.h"
#include "app/SystemAdminPage.h"
#include "core/auth/Session.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <functional>

namespace kb {

namespace {
// 深度优先遍历菜单树。
void walkMenus(const QVector<MenuItem> &items,
               const std::function<void(const MenuItem &)> &fn) {
    for (const MenuItem &m : items) {
        fn(m);
        if (!m.children.isEmpty()) {
            walkMenus(m.children, fn);
        }
    }
}
} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    buildUi();
    registerPages();
    buildNav();
}

void MainWindow::buildUi() {
    setWindowTitle(QStringLiteral("智能客服知识库系统"));
    resize(1080, 720);

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // 顶栏
    auto *topBar = new QFrame(central);
    topBar->setObjectName("TopBar");
    topBar->setFixedHeight(56);
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(20, 0, 16, 0);

    m_pageTitle = new QLabel(QString(), topBar);
    m_pageTitle->setObjectName("PageTitle");

    const CurrentUser &u = Session::instance().user();
    QString badge = u.realName.isEmpty() ? u.username
                                         : QStringLiteral("%1（%2）").arg(u.realName, u.username);
    auto *userBadge = new QLabel(badge, topBar);
    userBadge->setObjectName("UserBadge");

    auto *logout = new QPushButton(QStringLiteral("退出登录"), topBar);
    logout->setObjectName("GhostButton");
    logout->setCursor(Qt::PointingHandCursor);
    connect(logout, &QPushButton::clicked, this, &MainWindow::loggedOut);

    topLayout->addWidget(m_pageTitle);
    topLayout->addStretch();
    topLayout->addWidget(userBadge);
    topLayout->addSpacing(14);
    topLayout->addWidget(logout);

    // 主体：导航 + 内容
    auto *body = new QWidget(central);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    m_nav = new QTreeWidget(body);
    m_nav->setObjectName("NavTree");
    m_nav->setHeaderHidden(true);
    m_nav->setFixedWidth(220);
    m_nav->setIndentation(12);
    m_nav->setRootIsDecorated(false);
    m_nav->setFrameShape(QFrame::NoFrame);

    m_stack = new QStackedWidget(body);
    m_stack->setObjectName("ContentStack");

    bodyLayout->addWidget(m_nav);
    bodyLayout->addWidget(m_stack, 1);

    rootLayout->addWidget(topBar);
    rootLayout->addWidget(body, 1);

    setCentralWidget(central);

    m_router = new PageRouter(m_stack, this);

    connect(m_nav, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *, QTreeWidgetItem *) { navigateToCurrent(); });
}

void MainWindow::registerPages() {
    walkMenus(Session::instance().menus(), [this](const MenuItem &m) {
        if (m.name.isEmpty()) {
            return;
        }
        const QString title = m.title;
        const QString name = m.name;
        const bool phase2 = title.contains(QStringLiteral("二期"));
        m_router->registerPage(name, [title, name, phase2]() -> QWidget * {
            if (name == QStringLiteral("system.user") || name == QStringLiteral("system.role")
                || name == QStringLiteral("system.permission") || name == QStringLiteral("system.log")) {
                return new SystemAdminPage(title, name);
            }
            if (name == QStringLiteral("category")) {
                return new KnowledgeBasePage(title);
            }
            return new PlaceholderPage(title, name, phase2);
        });
    });
}

void MainWindow::buildNav() {
    m_nav->clear();

    std::function<void(const QVector<MenuItem> &, QTreeWidgetItem *)> add =
        [&](const QVector<MenuItem> &items, QTreeWidgetItem *parent) {
            for (const MenuItem &m : items) {
                auto *item = parent ? new QTreeWidgetItem(parent)
                                    : new QTreeWidgetItem(m_nav);
                item->setText(0, m.title);
                item->setData(0, Qt::UserRole, m.name);
                if (!m.children.isEmpty()) {
                    // 父节点仅用于分组，不可选中
                    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
                    add(m.children, item);
                    item->setExpanded(true);
                }
            }
        };
    add(Session::instance().menus(), nullptr);

    // 默认选中首个可导航项
    for (int i = 0; i < m_nav->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_nav->topLevelItem(i);
        const QString name = item->data(0, Qt::UserRole).toString();
        if (m_router->hasPage(name)) {
            m_nav->setCurrentItem(item);
            break;
        }
    }
}

void MainWindow::navigateToCurrent() {
    QTreeWidgetItem *item = m_nav->currentItem();
    if (!item) {
        return;
    }
    const QString name = item->data(0, Qt::UserRole).toString();
    if (name.isEmpty() || !m_router->hasPage(name)) {
        return;
    }
    m_router->navigate(name);
    m_pageTitle->setText(item->text(0));
}

} // namespace kb
