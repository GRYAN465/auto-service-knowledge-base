#pragma once

class QComboBox;

namespace kb {

/** 统一 QComboBox 弹出列表：无边框 QListView，配合 app.qss 使用。 */
class ComboStyle {
public:
    static void polish(QComboBox *box);
};

} // namespace kb
