#pragma once

#include <QDialogButtonBox>
#include <QDialog>
#include <QJsonArray>
#include <QJsonObject>

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTableWidget;
class QTextEdit;

namespace kb {

/**
 * 知识新建/编辑弹窗：标题 / 分类 / 类型 / 摘要 / 标签(可勾选) / 正文(富文本 + 工具栏) / 附件子面板。
 * 新建时附件面板在「保存草稿」后才可用（需要 articleId）。content 存 toHtml()。
 */
class ArticleEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ArticleEditorDialog(qint64 articleId, QWidget *parent = nullptr);

    /** 编辑过程中是否产生了需要外层刷新的变更（保存/附件增删）。 */
    bool dirty() const { return m_dirty; }

private:
    void buildUi();
    void loadCategories();
    void loadTags();
    void loadDetailIfEditing();

    void save();
    QJsonObject collectBody() const;

    // 附件子面板
    void refreshAttachments();
    void uploadAttachment();
    void downloadAttachment();
    void deleteAttachment();
    void setAttachmentPanelEnabled(bool enabled);
    qint64 selectedAttachmentId() const;

    void setStatus(const QString &text, bool error = false);

    qint64 m_articleId = 0;       // 0 = 新建（保存后回填）
    bool m_dirty = false;

    QLineEdit *m_title = nullptr;
    QComboBox *m_category = nullptr;
    QComboBox *m_type = nullptr;
    QLineEdit *m_summary = nullptr;
    QListWidget *m_tagList = nullptr;
    QTextEdit *m_content = nullptr;
    QLabel *m_status = nullptr;

    QPushButton *m_saveButton = nullptr;
    QTableWidget *m_attachTable = nullptr;
    QPushButton *m_attachUpload = nullptr;
    QPushButton *m_attachDownload = nullptr;
    QPushButton *m_attachDelete = nullptr;
    QLabel *m_attachHint = nullptr;
};

} // namespace kb
