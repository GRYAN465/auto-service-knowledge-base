#pragma once

#include "app/RefreshablePage.h"

#include <QWidget>

class QStackedWidget;

namespace kb {

class ArticleDetailPanel;
class UserProfilePanel;

/** 个人中心菜单页：主页 ↔ 详情栈。 */
class ProfilePage : public QWidget, public RefreshablePage {
    Q_OBJECT

public:
    explicit ProfilePage(const QString &title, QWidget *parent = nullptr);

    void refreshPage() override;

private:
    QStackedWidget *m_stack = nullptr;
    UserProfilePanel *m_profile = nullptr;
    ArticleDetailPanel *m_detail = nullptr;
};

} // namespace kb
