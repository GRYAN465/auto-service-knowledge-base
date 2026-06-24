#include "app/AuditCenterPage.h"

#include "app/RefreshablePage.h"
#include "common/TableStyle.h"
#include "core/auth/Session.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

namespace kb {

AuditCenterPage::AuditCenterPage(const QString &title, QWidget *parent)
    : QWidget(parent), m_title(title) {
    buildUi();
    refresh();
}

void AuditCenterPage::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 24);
    root->setSpacing(14);

    auto *toolbar = new QHBoxLayout();
    m_pass = new QPushButton(QStringLiteral("通过"), this);
    m_reject = new QPushButton(QStringLiteral("驳回"), this);
    m_pass->setObjectName("PrimaryButton");
    m_reject->setObjectName("GhostButton");
    toolbar->addWidget(m_pass);
    toolbar->addWidget(m_reject);
    toolbar->addStretch();
    root->addLayout(toolbar);

    const bool canAudit = Session::instance().hasPermission(QStringLiteral("knowledge:article:audit"));
    m_pass->setEnabled(canAudit);
    m_reject->setEnabled(canAudit);
    connect(m_pass, &QPushButton::clicked, this, &AuditCenterPage::pass);
    connect(m_reject, &QPushButton::clicked, this, &AuditCenterPage::reject);

    m_status = new QLabel(this);
    m_status->setObjectName("StatusLabel");
    root->addWidget(m_status);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    m_table = new QTableWidget(splitter);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({QStringLiteral("标题"), QStringLiteral("类型"),
                                        QStringLiteral("作者"), QStringLiteral("更新时间")});
    TableStyle::configureTitleTable(m_table, 0);
    splitter->addWidget(m_table);

    m_preview = new QTextEdit(splitter);
    m_preview->setObjectName("DataTable");
    m_preview->setReadOnly(true);
    m_preview->setPlaceholderText(QStringLiteral("选择左侧知识查看正文"));
    splitter->addWidget(m_preview);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    root->addWidget(splitter, 1);

    connect(m_table, &QTableWidget::itemSelectionChanged, this, &AuditCenterPage::previewSelected);
}

void AuditCenterPage::refreshPage() {
    refresh();
}

void AuditCenterPage::refresh() {
    setStatus(QStringLiteral("加载中..."));
    m_preview->clear();
    ApiClient::instance().get("/knowledge/article?status=PENDING_AUDIT&page=1&pageSize=50",
                              [this](const ApiResponse &r) {
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
            m_table->setItem(row, 1, new QTableWidgetItem(o.value("knowledgeType").toString()));
            m_table->setItem(row, 2, new QTableWidgetItem(o.value("authorName").toString()));
            m_table->setItem(row, 3, new QTableWidgetItem(o.value("updateTime").toString().replace('T', ' ')));
        }
        TableStyle::setItemTooltipFromText(m_table);
        setStatus(QStringLiteral("待审核 %1 条").arg(static_cast<qint64>(d.value("total").toDouble())));
    });
}

void AuditCenterPage::previewSelected() {
    const qint64 id = selectedId();
    if (id <= 0) {
        return;
    }
    ApiClient::instance().get("/knowledge/article/" + QString::number(id), [this](const ApiResponse &r) {
        if (!r.ok) {
            return;
        }
        const QJsonObject d = r.object();
        QString html = QStringLiteral("<h3>%1</h3>").arg(d.value("title").toString().toHtmlEscaped());
        const QString summary = d.value("summary").toString();
        if (!summary.isEmpty()) {
            html += QStringLiteral("<p style='color:#757575'>%1</p><hr>").arg(summary.toHtmlEscaped());
        }
        html += d.value("content").toString();
        m_preview->setHtml(html);
    });
}

void AuditCenterPage::pass() {
    if (selectedId() <= 0) {
        setStatus(QStringLiteral("请先选择一条待审核知识"), true);
        return;
    }
    QJsonObject body;
    body["auditStatus"] = "PASS";
    body["opinion"] = QStringLiteral("通过");
    doAudit(body, QStringLiteral("已通过并上线"));
}

void AuditCenterPage::reject() {
    if (selectedId() <= 0) {
        setStatus(QStringLiteral("请先选择一条待审核知识"), true);
        return;
    }
    bool ok = false;
    const QString opinion = QInputDialog::getText(this, QStringLiteral("驳回"),
                                                  QStringLiteral("驳回意见："),
                                                  QLineEdit::Normal, QString(), &ok);
    if (!ok) {
        return;
    }
    if (opinion.trimmed().isEmpty()) {
        setStatus(QStringLiteral("请填写驳回意见"), true);
        return;
    }
    QJsonObject body;
    body["auditStatus"] = "REJECT";
    body["opinion"] = opinion.trimmed();
    doAudit(body, QStringLiteral("已驳回"));
}

void AuditCenterPage::doAudit(const QJsonObject &body, const QString &okText) {
    const qint64 id = selectedId();
    ApiClient::instance().post("/knowledge/article/" + QString::number(id) + "/audit", body,
                               [this, okText](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        setStatus(okText);
        refresh();
    });
}

qint64 AuditCenterPage::selectedId() const {
    const int row = m_table->currentRow();
    if (row < 0 || !m_table->item(row, 0)) {
        return 0;
    }
    return m_table->item(row, 0)->data(Qt::UserRole).toLongLong();
}

QString AuditCenterPage::selectedTitle() const {
    const int row = m_table->currentRow();
    return (row < 0 || !m_table->item(row, 0)) ? QString() : m_table->item(row, 0)->text();
}

void AuditCenterPage::setStatus(const QString &text, bool error) {
    m_status->setText(text);
    m_status->setStyleSheet(error ? "color:#B94A48;" : "color:#757575;");
    if (error && !text.isEmpty()) {
        notify::warn(this, text);
    }
}

} // namespace kb
