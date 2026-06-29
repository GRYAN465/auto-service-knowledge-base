#include "common/ComboStyle.h"

#include <QComboBox>
#include <QFrame>
#include <QListView>
#include <QAbstractItemView>

namespace kb {

void ComboStyle::polish(QComboBox *box) {
    if (!box || box->property("kbComboPolished").toBool()) {
        return;
    }
    auto *view = new QListView(box);
    view->setObjectName(QStringLiteral("ComboPopupList"));
    view->setFrameShape(QFrame::NoFrame);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setSpacing(0);
    box->setView(view);
    box->setProperty("kbComboPolished", true);
}

} // namespace kb
