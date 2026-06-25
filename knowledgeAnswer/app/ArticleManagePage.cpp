#include "app/ArticleManagePage.h"

#include "app/ArticleEditorDialog.h"
#include "common/TableStyle.h"
#include "core/auth/Session.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace kb {

namespace {

QString statusLabel(const QString &code) {
    if (code == "DRAFT") return QStringLiteral("草稿");
    if (code == "PENDING_AUDIT") return QStringLiteral("待审核");
    if (code == "ONLINE") return QStringLiteral("已上线");
    if (code == "OFFLINE") return QStringLiteral("已下线");
    if (code == "REJECTED") return QStringLiteral("已驳回");
    return code;
}

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

ArticleManagePage::ArticleManagePage(const QString &title, QWidget *parent)
    : QWidget(parent), m_title(title) {
    buildUi();
    loadCategoryFilter();
    refresh();
}

void ArticleManagePage::refreshPage() {
    refresh();
}

void ArticleManagePage::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 24);
    root->setSpacing(14);

    // 筛选栏
    auto *filter = new QHBoxLayout();
    m_keyword = new QLineEdit(this);
    m_keyword->setPlaceholderText(QStringLiteral("标题关键词"));
    m_keyword->setMaximumWidth(220);
    m_categoryFilter = new QComboBox(this);
    m_categoryFilter->addItem(QStringLiteral("全部分类"), QVariant::fromValue<qint64>(0));
    m_statusFilter = new QComboBox(this);
    m_statusFilter->addItem(QStringLiteral("全部状态"), QString());
    for (const char *s : {"DRAFT", "PENDING_AUDIT", "ONLINE", "OFFLINE", "REJECTED"}) {
        m_statusFilter->addItem(statusLabel(QString::fromLatin1(s)), QString::fromLatin1(s));
    }
    m_typeFilter = new QComboBox(this);
    m_typeFilter->addItem(QStringLiteral("全部类型"), QString());
    for (const char *t : {"SCRIPT", "TRAIN", "PRODUCT", "OFFICE"}) {
        m_typeFilter->addItem(typeLabel(QString::fromLatin1(t)), QString::fromLatin1(t));
    }
    auto *query = new QPushButton(QStringLiteral("查询"), this);
    query->setObjectName("PrimaryButton");
    filter->addWidget(new QLabel(QStringLiteral("筛选："), this));
    filter->addWidget(m_keyword);
    filter->addWidget(m_categoryFilter);
    filter->addWidget(m_statusFilter);
    filter->addWidget(m_typeFilter);
    filter->addWidget(query);
    filter->addStretch();
    root->addLayout(filter);
    connect(query, &QPushButton::clicked, this, &ArticleManagePage::refresh);

    // 工具栏
    auto *toolbar = new QHBoxLayout();
    m_create = new QPushButton(QStringLiteral("新增"), this);
    m_edit = new QPushButton(QStringLiteral("编辑"), this);
    m_delete = new QPushButton(QStringLiteral("删除"), this);
    m_submit = new QPushButton(QStringLiteral("提交审核"), this);
    m_online = new QPushButton(QStringLiteral("上线"), this);
    m_offline = new QPushButton(QStringLiteral("下线"), this);
    m_create->setObjectName("PrimaryButton");
    for (QPushButton *b : {m_edit, m_delete, m_submit, m_online, m_offline}) {
        b->setObjectName("GhostButton");
    }
    toolbar->addWidget(m_create);
    toolbar->addWidget(m_edit);
    toolbar->addWidget(m_delete);
    toolbar->addWidget(m_submit);
    toolbar->addWidget(m_online);
    toolbar->addWidget(m_offline);
    toolbar->addStretch();
    root->addLayout(toolbar);

    auto &session = Session::instance();
    m_create->setEnabled(session.hasPermission(QStringLiteral("knowledge:article:create")));
    m_edit->setEnabled(session.hasPermission(QStringLiteral("knowledge:article:update")));
    m_delete->setEnabled(session.hasPermission(QStringLiteral("knowledge:article:delete")));
    m_submit->setEnabled(session.hasPermission(QStringLiteral("knowledge:article:submit")));
    m_online->setEnabled(session.hasPermission(QStringLiteral("knowledge:article:online")));
    m_offline->setEnabled(session.hasPermission(QStringLiteral("knowledge:article:offline")));
    connect(m_create, &QPushButton::clicked, this, &ArticleManagePage::createArticle);
    connect(m_edit, &QPushButton::clicked, this, &ArticleManagePage::editArticle);
    connect(m_delete, &QPushButton::clicked, this, &ArticleManagePage::deleteArticle);
    connect(m_submit, &QPushButton::clicked, this, &ArticleManagePage::submitArticle);
    connect(m_online, &QPushButton::clicked, this, &ArticleManagePage::onlineArticle);
    connect(m_offline, &QPushButton::clicked, this, &ArticleManagePage::offlineArticle);

    m_status = new QLabel(this);
    m_status->setObjectName("StatusLabel");
    root->addWidget(m_status);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({QStringLiteral("标题"), QStringLiteral("分类"),
                                        QStringLiteral("类型"), QStringLiteral("状态"),
                                        QStringLiteral("作者"), QStringLiteral("浏览"),
                                        QStringLiteral("更新时间")});
    TableStyle::configureTitleTable(m_table, 0);
    root->addWidget(m_table, 1);
    connect(m_table, &QTableWidget::doubleClicked, this, &ArticleManagePage::editArticle);
}

void ArticleManagePage::loadCategoryFilter() {
    ApiClient::instance().get("/knowledge/category/tree", [this](const ApiResponse &r) {
        if (r.ok) {
            flatten(m_categoryFilter, r.data.toArray(), 0);
        }
    });
}

void ArticleManagePage::refresh() {
    QStringList params;
    params << "page=1" << "pageSize=50";
    if (!m_keyword->text().trimmed().isEmpty()) {
        params << "keyword=" + QString::fromUtf8(QUrl::toPercentEncoding(m_keyword->text().trimmed()));
    }
    if (m_categoryFilter->currentData().toLongLong() > 0) {
        params << "categoryId=" + QString::number(m_categoryFilter->currentData().toLongLong());
    }
    if (!m_statusFilter->currentData().toString().isEmpty()) {
        params << "status=" + m_statusFilter->currentData().toString();
    }
    if (!m_typeFilter->currentData().toString().isEmpty()) {
        params << "knowledgeType=" + m_typeFilter->currentData().toString();
    }
    setStatus(QStringLiteral("加载中..."));
    ApiClient::instance().get("/knowledge/article?" + params.join('&'), [this](const ApiResponse &r) {
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
            titleItem->setData(Qt::UserRole + 1, o.value("status").toString());
            m_table->setItem(row, 0, titleItem);
            m_table->setItem(row, 1, new QTableWidgetItem(o.value("categoryName").toString()));
            m_table->setItem(row, 2, new QTableWidgetItem(typeLabel(o.value("knowledgeType").toString())));
            m_table->setItem(row, 3, new QTableWidgetItem(statusLabel(o.value("status").toString())));
            m_table->setItem(row, 4, new QTableWidgetItem(o.value("authorName").toString()));
            m_table->setItem(row, 5, new QTableWidgetItem(
                QString::number(static_cast<qint64>(o.value("viewCount").toDouble()))));
            m_table->setItem(row, 6, new QTableWidgetItem(o.value("updateTime").toString().replace('T', ' ')));
        }
        TableStyle::setItemTooltipFromText(m_table);
        setStatus(QStringLiteral("共 %1 条").arg(static_cast<qint64>(d.value("total").toDouble())));
    });
}

void ArticleManagePage::createArticle() {
    ArticleEditorDialog dlg(0, this);
    dlg.exec();
    if (dlg.dirty()) {
        refresh();
    }
}

void ArticleManagePage::editArticle() {
    const qint64 id = selectedId();
    if (id <= 0) {
        setStatus(QStringLiteral("请先选择一条知识"), true);
        return;
    }
    ArticleEditorDialog dlg(id, this);
    dlg.exec();
    if (dlg.dirty()) {
        refresh();
    }
}

void ArticleManagePage::deleteArticle() {
    const qint64 id = selectedId();
    if (id <= 0) {
        setStatus(QStringLiteral("请先选择一条知识"), true);
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("确认删除"),
                              QStringLiteral("确认删除知识「%1」？").arg(selectedTitle()))
        != QMessageBox::Yes) {
        return;
    }
    ApiClient::instance().del("/knowledge/article/" + QString::number(id), [this](const ApiResponse &r) {
        r.ok ? refresh() : setStatus(r.message, true);
    });
}

void ArticleManagePage::submitArticle() {
    if (selectedId() <= 0) {
        setStatus(QStringLiteral("请先选择一条知识"), true);
        return;
    }
    doAction("submit", {}, QStringLiteral("已提交审核"));
}

void ArticleManagePage::onlineArticle() {
    if (selectedId() <= 0) {
        setStatus(QStringLiteral("请先选择一条知识"), true);
        return;
    }
    doAction("online", {}, QStringLiteral("已上线"));
}

void ArticleManagePage::offlineArticle() {
    if (selectedId() <= 0) {
        setStatus(QStringLiteral("请先选择一条知识"), true);
        return;
    }
    bool ok = false;
    const QString reason = QInputDialog::getText(this, QStringLiteral("下线"),
                                                 QStringLiteral("下线原因（可选）："),
                                                 QLineEdit::Normal, QString(), &ok);
    if (!ok) {
        return;
    }
    QJsonObject body;
    body["reason"] = reason;
    doAction("offline", body, QStringLiteral("已下线"));
}

void ArticleManagePage::doAction(const QString &subPath, const QJsonObject &body, const QString &okText) {
    const qint64 id = selectedId();
    ApiClient::instance().post("/knowledge/article/" + QString::number(id) + "/" + subPath, body,
                               [this, okText](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        setStatus(okText);
        refresh();
    });
}

qint64 ArticleManagePage::selectedId() const {
    const int row = m_table->currentRow();
    if (row < 0 || !m_table->item(row, 0)) {
        return 0;
    }
    return m_table->item(row, 0)->data(Qt::UserRole).toLongLong();
}

QString ArticleManagePage::selectedStatus() const {
    const int row = m_table->currentRow();
    return (row < 0 || !m_table->item(row, 0)) ? QString()
                                               : m_table->item(row, 0)->data(Qt::UserRole + 1).toString();
}

QString ArticleManagePage::selectedTitle() const {
    const int row = m_table->currentRow();
    return (row < 0 || !m_table->item(row, 0)) ? QString() : m_table->item(row, 0)->text();
}

void ArticleManagePage::setStatus(const QString &text, bool error) {
    m_status->setText(text);
    m_status->setStyleSheet(error ? "color:#B94A48;" : "color:#757575;");
    if (error && !text.isEmpty()) {
        notify::warn(this, text);
    }
}

} // namespace kb
