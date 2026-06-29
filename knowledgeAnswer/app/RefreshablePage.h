#pragma once

namespace kb {

/** 可由主窗口顶栏「刷新」触发的页面。 */
class RefreshablePage {
public:
    virtual ~RefreshablePage() = default;
    virtual void refreshPage() = 0;
};

} // namespace kb
