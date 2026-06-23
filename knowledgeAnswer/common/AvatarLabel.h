#pragma once

#include <QLabel>

namespace kb {

/** 圆形头像：蓝底白字，显示姓名/用户名首字。 */
class AvatarLabel : public QLabel {
    Q_OBJECT

public:
    explicit AvatarLabel(int sizePx, QWidget *parent = nullptr);

    void setDisplayName(const QString &name);

private:
    int m_sizePx = 48;
};

} // namespace kb
