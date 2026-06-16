#pragma once

#include <QDialog>

class QComboBox;
class QTextEdit;

namespace kb {

/**
 * 分享知识给同事（模块 6）：
 *   - 打开即 GET /interaction/users 填充收件人下拉（id+姓名，避开 system:user:list 权限）
 *   - 选收件人 + 填留言 → POST /interaction/share，成功后 accept
 * 复用 PrimaryButton/GhostButton objectName，不引新样式。
 */
class ShareDialog : public QDialog {
    Q_OBJECT

public:
    explicit ShareDialog(qint64 articleId, QWidget *parent = nullptr);

private:
    void buildUi();
    void loadUsers();
    void submit();

    qint64 m_articleId = 0;
    QComboBox *m_recipient = nullptr;
    QTextEdit *m_message = nullptr;
};

} // namespace kb
