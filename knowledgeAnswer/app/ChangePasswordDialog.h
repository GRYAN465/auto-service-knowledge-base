#pragma once

#include <QDialog>

class QLineEdit;

namespace kb {

class ChangePasswordDialog : public QDialog {
    Q_OBJECT

public:
    explicit ChangePasswordDialog(QWidget *parent = nullptr);

private:
    QLineEdit *m_oldPwd = nullptr;
    QLineEdit *m_newPwd = nullptr;
};

} // namespace kb
