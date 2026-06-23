#pragma once

#include <QDialog>

namespace kb {

class ArticleDetailPanel;

/** 知识详情弹窗：内嵌 {@link ArticleDetailPanel}，与社区/个人中心详情能力一致。 */
class ArticleDetailDialog : public QDialog {
    Q_OBJECT

public:
    explicit ArticleDetailDialog(qint64 articleId, QWidget *parent = nullptr);

private:
    ArticleDetailPanel *m_panel = nullptr;
};

} // namespace kb
