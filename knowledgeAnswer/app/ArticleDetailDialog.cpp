#include "app/ArticleDetailDialog.h"

#include "app/ArticleDetailPanel.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace kb {

ArticleDetailDialog::ArticleDetailDialog(qint64 articleId, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("知识详情"));
    resize(760, 720);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_panel = new ArticleDetailPanel(this);
    m_panel->setBackButtonVisible(false);
    root->addWidget(m_panel, 1);

    auto *footer = new QHBoxLayout();
    footer->setContentsMargins(24, 8, 24, 16);
    footer->addStretch();
    auto *close = new QPushButton(QStringLiteral("关闭"), this);
    close->setObjectName("GhostButton");
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    footer->addWidget(close);
    root->addLayout(footer);

    m_panel->showArticle(articleId);
}

} // namespace kb
