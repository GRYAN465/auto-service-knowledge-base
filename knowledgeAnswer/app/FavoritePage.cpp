#include "app/FavoritePage.h"

#include "app/ArticleDetailDialog.h"
#include "app/RefreshablePage.h"
#include "common/TableStyle.h"
#include "common/ThemeIcons.h"
#include "core/auth/Session.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace kb {

namespace {

QString typeLabel(const QString &code) {
    if (code == "SCRIPT") return QStringLiteral("话术");
    if (code == "TRAIN") return QStringLiteral("培训");
    if (code == "PRODUCT") return QStringLiteral("产品");
    if (code == "OFFICE") return QStringLiteral("办公");
    return code;
}

} // namespace

FavoritePage::FavoritePage(const QString &title, QWidget *parent)
    : QWidget(parent), m_title(title) {
    buildUi();
    refresh();
}

void FavoritePage::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 24);
    root->setSpacing(14);

    auto *toolbar = new QHBoxLayout();
    m_unfavBtn = new QPushButton(this);
    ThemeIcons::applyIconButton(m_unfavBtn, ThemeIcons::Kind::StarFilled, QStringLiteral("取消收藏"));
    m_unfavBtn->setEnabled(Session::instance().hasPermission("interaction:favorite"));
    connect(m_unfavBtn, &QPushButton::clicked, this, &FavoritePage::unfavorite);
    toolbar->addWidget(m_unfavBtn);
    toolbar->addStretch();
    root->addLayout(toolbar);

    m_status = new QLabel(this);
    m_status->setObjectName("StatusLabel");
    root->addWidget(m_status);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({QStringLiteral("标题"), QStringLiteral("分类"),
                                        QStringLiteral("类型"), QStringLiteral("收藏夹"),
                                        QStringLiteral("收藏时间")});
    TableStyle::configureTitleTable(m_table, 0);
    root->addWidget(m_table, 1);
    connect(m_table, &QTableWidget::doubleClicked, this, &FavoritePage::openDetail);
}

void FavoritePage::refreshPage() {
    refresh();
}

void FavoritePage::refresh() {
    setStatus(QStringLiteral("加载中..."));
    ApiClient::instance().get("/interaction/favorite?page=1&pageSize=50", [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        const QJsonObject d = r.object();
        const QJsonArray list = d.value("list").toArray();
        m_table->setRowCount(0);
        for (const QJsonValue &v : list) {
            const QJsonObject o = v.toObject();
            const int row = m_table->rowCount();
            m_table->insertRow(row);
            auto *titleItem = new QTableWidgetItem(o.value("title").toString());
            titleItem->setData(Qt::UserRole, o.value("articleId").toVariant());
            m_table->setItem(row, 0, titleItem);
            m_table->setItem(row, 1, new QTableWidgetItem(o.value("categoryName").toString()));
            m_table->setItem(row, 2, new QTableWidgetItem(typeLabel(o.value("knowledgeType").toString())));
            m_table->setItem(row, 3, new QTableWidgetItem(o.value("folder").toString()));
            m_table->setItem(row, 4, new QTableWidgetItem(o.value("createTime").toString().replace('T', ' ')));
        }
        TableStyle::setItemTooltipFromText(m_table);
        setStatus(QStringLiteral("共 %1 条收藏").arg(static_cast<qint64>(d.value("total").toDouble())));
    });
}

void FavoritePage::openDetail() {
    const qint64 id = selectedArticleId();
    if (id <= 0) {
        return;
    }
    ArticleDetailDialog dlg(id, this);
    dlg.exec();
    refresh();
}

void FavoritePage::unfavorite() {
    const qint64 id = selectedArticleId();
    if (id <= 0) {
        notify::warn(this, QStringLiteral("请先选中一条收藏"));
        return;
    }
    ApiClient::instance().del("/interaction/favorite/" + QString::number(id), [this](const ApiResponse &r) {
        if (!r.ok) {
            notify::warn(this, r.message);
            return;
        }
        refresh();
    });
}

qint64 FavoritePage::selectedArticleId() const {
    const int row = m_table->currentRow();
    if (row < 0 || !m_table->item(row, 0)) {
        return 0;
    }
    return m_table->item(row, 0)->data(Qt::UserRole).toLongLong();
}

void FavoritePage::setStatus(const QString &text, bool error) {
    m_status->setText(text);
    m_status->setStyleSheet(error ? "color:#B94A48;" : "color:#757575;");
    if (error && !text.isEmpty()) {
        notify::warn(this, text);
    }
}

} // namespace kb
