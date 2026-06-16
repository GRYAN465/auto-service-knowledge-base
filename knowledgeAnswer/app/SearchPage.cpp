#include "app/SearchPage.h"

#include "app/ArticleDetailDialog.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>
#include <QTableWidget>
#include <QUrl>
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

void flatten(QComboBox *box, const QJsonArray &nodes, int depth) {
    for (const QJsonValue &v : nodes) {
        const QJsonObject o = v.toObject();
        box->addItem(QString(depth * 2, QChar(' ')) + o.value("name").toString(),
                     QVariant::fromValue<qint64>(static_cast<qint64>(o.value("id").toDouble())));
        flatten(box, o.value("children").toArray(), depth + 1);
    }
}

} // namespace

SearchPage::SearchPage(const QString &title, QWidget *parent)
    : QWidget(parent), m_title(title) {
    buildUi();
    loadFilters();
    refresh();
}

void SearchPage::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 24);
    root->setSpacing(14);

    // 筛选栏
    auto *filter = new QHBoxLayout();
    m_keyword = new QLineEdit(this);
    m_keyword->setPlaceholderText(QStringLiteral("搜索标题 / 摘要 / 正文"));
    m_keyword->setClearButtonEnabled(true);
    m_keyword->setMaximumWidth(280);
    m_categoryFilter = new QComboBox(this);
    m_categoryFilter->addItem(QStringLiteral("全部分类"), QVariant::fromValue<qint64>(0));
    m_tagFilter = new QComboBox(this);
    m_tagFilter->addItem(QStringLiteral("全部标签"), QVariant::fromValue<qint64>(0));
    auto *query = new QPushButton(QStringLiteral("检索"), this);
    query->setObjectName("PrimaryButton");
    filter->addWidget(new QLabel(QStringLiteral("筛选："), this));
    filter->addWidget(m_keyword);
    filter->addWidget(m_categoryFilter);
    filter->addWidget(m_tagFilter);
    filter->addWidget(query);
    filter->addStretch();
    root->addLayout(filter);
    connect(query, &QPushButton::clicked, this, &SearchPage::refresh);
    connect(m_keyword, &QLineEdit::returnPressed, this, &SearchPage::refresh);

    m_status = new QLabel(this);
    m_status->setObjectName("StatusLabel");
    root->addWidget(m_status);

    m_table = new QTableWidget(this);
    m_table->setObjectName("DataTable");
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({QStringLiteral("标题"), QStringLiteral("分类"),
                                        QStringLiteral("类型"), QStringLiteral("作者"),
                                        QStringLiteral("浏览"), QStringLiteral("更新时间")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setColumnWidth(0, 300);
    root->addWidget(m_table, 1);
    connect(m_table, &QTableWidget::doubleClicked, this, &SearchPage::openDetail);
}

void SearchPage::loadFilters() {
    ApiClient::instance().get("/knowledge/category/tree", [this](const ApiResponse &r) {
        if (r.ok) {
            flatten(m_categoryFilter, r.data.toArray(), 0);
        }
    });
    ApiClient::instance().get("/knowledge/tag?page=1&pageSize=200", [this](const ApiResponse &r) {
        if (!r.ok) {
            return;
        }
        const QJsonArray list = r.object().value("list").toArray();
        for (const QJsonValue &v : list) {
            const QJsonObject o = v.toObject();
            m_tagFilter->addItem(o.value("name").toString(),
                                 QVariant::fromValue<qint64>(static_cast<qint64>(o.value("id").toDouble())));
        }
    });
}

void SearchPage::refresh() {
    QStringList params;
    params << "page=1" << "pageSize=50";
    if (!m_keyword->text().trimmed().isEmpty()) {
        params << "keyword=" + QString::fromUtf8(QUrl::toPercentEncoding(m_keyword->text().trimmed()));
    }
    if (m_categoryFilter->currentData().toLongLong() > 0) {
        params << "categoryId=" + QString::number(m_categoryFilter->currentData().toLongLong());
    }
    if (m_tagFilter->currentData().toLongLong() > 0) {
        params << "tagId=" + QString::number(m_tagFilter->currentData().toLongLong());
    }
    setStatus(QStringLiteral("检索中..."));
    ApiClient::instance().get("/search/article?" + params.join('&'), [this](const ApiResponse &r) {
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
            titleItem->setData(Qt::UserRole, o.value("id").toVariant());
            m_table->setItem(row, 0, titleItem);
            m_table->setItem(row, 1, new QTableWidgetItem(o.value("categoryName").toString()));
            m_table->setItem(row, 2, new QTableWidgetItem(typeLabel(o.value("knowledgeType").toString())));
            m_table->setItem(row, 3, new QTableWidgetItem(o.value("authorName").toString()));
            m_table->setItem(row, 4, new QTableWidgetItem(
                QString::number(static_cast<qint64>(o.value("viewCount").toDouble()))));
            m_table->setItem(row, 5, new QTableWidgetItem(o.value("updateTime").toString().replace('T', ' ')));
        }
        setStatus(QStringLiteral("共 %1 条").arg(static_cast<qint64>(d.value("total").toDouble())));
    });
}

void SearchPage::openDetail() {
    const qint64 id = selectedId();
    if (id <= 0) {
        return;
    }
    ArticleDetailDialog dlg(id, this);
    dlg.exec();
    // 浏览数已在详情弹窗内自增，回到列表刷新以反映最新浏览数。
    refresh();
}

qint64 SearchPage::selectedId() const {
    const int row = m_table->currentRow();
    if (row < 0 || !m_table->item(row, 0)) {
        return 0;
    }
    return m_table->item(row, 0)->data(Qt::UserRole).toLongLong();
}

void SearchPage::setStatus(const QString &text, bool error) {
    m_status->setText(text);
    m_status->setStyleSheet(error ? "color:#DC2626;" : "color:#6B7280;");
    if (error && !text.isEmpty()) {
        notify::warn(this, text);
    }
}

} // namespace kb
