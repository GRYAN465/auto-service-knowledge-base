#include "app/MainWindow.h"

#include "app/RefreshablePage.h"

#include "app/AiConfigPage.h"
#include "app/ArticleManagePage.h"
#include "app/AuditCenterPage.h"
#include "app/DashboardPage.h"
#include "app/FavoritePage.h"
#include "app/KnowledgeBasePage.h"
#include "app/OpenApiPage.h"
#include "app/PageRouter.h"
#include "app/PlaceholderPage.h"
#include "app/ProfilePage.h"
#include "app/QaPage.h"
#include "app/SearchPage.h"
#include "app/SharePage.h"
#include "app/StatisticsPage.h"
#include "app/SystemAdminPage.h"
#include "common/NavTreeDelegate.h"
#include "core/auth/Session.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
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

QTreeWidgetItem *findNavItemByRoute(QTreeWidget *nav, const QString &route) {
    if (!nav || route.isEmpty()) {
        return nullptr;
    }
    for (QTreeWidgetItemIterator it(nav); *it; ++it) {
        if ((*it)->data(0, Qt::UserRole).toString() == route) {
            return *it;
        }
    }
    return nullptr;
}

QTreeWidgetItem *findFirstNavigableItem(QTreeWidget *nav, const PageRouter *router) {
    if (!nav || !router) {
        return nullptr;
    }
    for (QTreeWidgetItemIterator it(nav); *it; ++it) {
        QTreeWidgetItem *item = *it;
        const QString route = item->data(0, Qt::UserRole).toString();
        if (route.isEmpty() || !router->hasPage(route)) {
            continue;
        }
        if (item->flags() & Qt::ItemIsSelectable) {
            return item;
        }
    }
    return nullptr;
}

QTreeWidgetItem *pickInitialNavItem(QTreeWidget *nav, const PageRouter *router) {
    static const QStringList preferred = {
        QStringLiteral("dashboard"),
        QStringLiteral("knowledge.search"),
    };
    for (const QString &route : preferred) {
        if (!router->hasPage(route)) {
            continue;
        }
        if (QTreeWidgetItem *item = findNavItemByRoute(nav, route)) {
            return item;
        }
    }
    return findFirstNavigableItem(nav, router);
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

    m_refreshBtn = new QPushButton(QStringLiteral("刷新"), topBar);
    m_refreshBtn->setObjectName("GhostButton");
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    m_refreshBtn->setVisible(false);
    connect(m_refreshBtn, &QPushButton::clicked, this, [this]() {
        if (auto *page = dynamic_cast<RefreshablePage *>(m_router->currentWidget())) {
            page->refreshPage();
        }
    });

    const CurrentUser &u = Session::instance().user();
    QString badge = u.realName.isEmpty() ? u.username
                                         : QStringLiteral("%1（%2）").arg(u.realName, u.username);
    auto *userBtn = new QPushButton(badge, topBar);
    userBtn->setObjectName("GhostButton");
    userBtn->setFlat(true);
    userBtn->setCursor(Qt::PointingHandCursor);
    connect(userBtn, &QPushButton::clicked, this, [this]() { navigateToRoute(QStringLiteral("personal.center")); });

    auto *logout = new QPushButton(QStringLiteral("退出登录"), topBar);
    logout->setObjectName("GhostButton");
    logout->setCursor(Qt::PointingHandCursor);
    connect(logout, &QPushButton::clicked, this, &MainWindow::loggedOut);

    topLayout->addWidget(m_pageTitle);
    topLayout->addSpacing(12);
    topLayout->addWidget(m_refreshBtn);
    topLayout->addStretch();

    const QVector<QString> &roles = Session::instance().roles();
    if (!roles.isEmpty()) {
        QString roleText = roles.first();
        if (roleText == QStringLiteral("ADMIN")) {
            roleText = QStringLiteral("管理员");
        } else if (roleText == QStringLiteral("KNOWLEDGE_ADMIN")) {
            roleText = QStringLiteral("知识管理员");
        } else if (roleText == QStringLiteral("AUDITOR")) {
            roleText = QStringLiteral("审核员");
        } else if (roleText == QStringLiteral("AGENT")) {
            roleText = QStringLiteral("坐席");
        } else if (roleText == QStringLiteral("USER")) {
            roleText = QStringLiteral("普通用户");
        }
        auto *roleBadge = new QLabel(roleText, topBar);
        roleBadge->setObjectName("RoleBadge");
        topLayout->addWidget(roleBadge);
        topLayout->addSpacing(12);
    }

    topLayout->addWidget(userBtn);
    topLayout->addSpacing(14);
    topLayout->addWidget(logout);

    // 主体：导航 + 内容
    auto *body = new QWidget(central);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    auto *navSidebar = new QFrame(body);
    navSidebar->setObjectName("NavSidebar");
    navSidebar->setFixedWidth(220);
    auto *navLayout = new QVBoxLayout(navSidebar);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(0);

    m_nav = new QTreeWidget(navSidebar);
    m_nav->setObjectName("NavTree");
    m_nav->setHeaderHidden(true);
    m_nav->setIndentation(14);
    m_nav->setRootIsDecorated(false);
    m_nav->setFrameShape(QFrame::NoFrame);
    m_nav->setItemDelegate(new NavTreeDelegate(m_nav));
    m_nav->setMouseTracking(true);
    navLayout->addWidget(m_nav);

    m_stack = new QStackedWidget(body);
    m_stack->setObjectName("ContentStack");

    bodyLayout->addWidget(navSidebar);
    bodyLayout->addWidget(m_stack, 1);

    rootLayout->addWidget(topBar);
    rootLayout->addWidget(body, 1);

    setCentralWidget(central);

    m_router = new PageRouter(m_stack, this);
    connect(m_router, &PageRouter::pageShown, this, [this](const QString &, bool firstVisit) {
        updateTopBarRefresh();
        if (firstVisit) {
            return;
        }
        if (auto *page = dynamic_cast<RefreshablePage *>(m_router->currentWidget())) {
            page->refreshPage();
        }
    });

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
        m_router->registerPage(name, [this, title, name, phase2]() -> QWidget * {
            if (name == QStringLiteral("dashboard")) {
                auto *page = new DashboardPage(title);
                connect(page, &DashboardPage::navigateRequested, this, &MainWindow::navigateToRoute);
                return page;
            }
            if (name == QStringLiteral("system.user") || name == QStringLiteral("system.role")
                || name == QStringLiteral("system.permission") || name == QStringLiteral("system.log")) {
                return new SystemAdminPage(title, name);
            }
            if (name == QStringLiteral("category")) {
                return new KnowledgeBasePage(title);
            }
            if (name == QStringLiteral("knowledge.manage")) {
                return new ArticleManagePage(title);
            }
            if (name == QStringLiteral("knowledge.search")) {
                return new SearchPage(title);
            }
            if (name == QStringLiteral("personal.center")) {
                return new ProfilePage(title);
            }
            if (name == QStringLiteral("favorite")) {
                return new FavoritePage(title);
            }
            if (name == QStringLiteral("share")) {
                return new SharePage(title);
            }
            if (name == QStringLiteral("audit")) {
                return new AuditCenterPage(title);
            }
            if (name == QStringLiteral("statistics")) {
                return new StatisticsPage(title);
            }
            if (name == QStringLiteral("openapi")) {
                return new OpenApiPage(title);
            }
            if (name == QStringLiteral("qa")) {
                return new QaPage(title);
            }
            if (name == QStringLiteral("ai.config")) {
                return new AiConfigPage(title);
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
                    item->setData(0, NavTreeDelegate::NavItemKindRole, QStringLiteral("group"));
                    // 父节点仅用于分组，不可选中
                    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
                    add(m.children, item);
                    item->setExpanded(true);
                } else {
                    item->setData(0, NavTreeDelegate::NavItemKindRole, QStringLiteral("link"));
                }
            }
        };
    add(Session::instance().menus(), nullptr);

    if (QTreeWidgetItem *initial = pickInitialNavItem(m_nav, m_router)) {
        m_nav->setCurrentItem(initial);
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

void MainWindow::updateTopBarRefresh() {
    if (!m_refreshBtn || !m_router) {
        return;
    }
    const bool canRefresh = dynamic_cast<RefreshablePage *>(m_router->currentWidget()) != nullptr;
    m_refreshBtn->setVisible(canRefresh);
}

void MainWindow::navigateToRoute(const QString &name) {
    if (name.isEmpty()) {
        return;
    }
    // 找到路由名匹配的导航树项并选中——复用 currentItemChanged → navigateToCurrent 流，
    // 让左侧高亮与页面标题随之同步。找不到（该角色菜单无此项）则忽略。
    for (QTreeWidgetItemIterator it(m_nav); *it; ++it) {
        if ((*it)->data(0, Qt::UserRole).toString() == name) {
            m_nav->setCurrentItem(*it);
            return;
        }
    }
}

} // namespace kb
