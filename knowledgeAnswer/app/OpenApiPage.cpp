#include "app/OpenApiPage.h"

#include "app/RefreshablePage.h"
#include "common/TableStyle.h"

#include "core/auth/Session.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>

namespace kb {

namespace {

QString textOf(const QJsonObject &o, const QString &key) {
    const QJsonValue v = o.value(key);
    if (v.isDouble()) {
        return QString::number(static_cast<qint64>(v.toDouble()));
    }
    return v.toString();
}

QString timeText(const QJsonObject &o, const QString &key) {
    return o.value(key).toString().replace('T', ' ');
}

QLineEdit *line(QFormLayout *form, const QString &label, const QString &value = QString()) {
    auto *edit = new QLineEdit();
    edit->setText(value);
    form->addRow(label, edit);
    return edit;
}

QComboBox *statusBox(QFormLayout *form, const QString &value) {
    auto *box = new QComboBox();
    box->addItems({QStringLiteral("ENABLED"), QStringLiteral("DISABLED")});
    const int idx = box->findText(value);
    box->setCurrentIndex(idx < 0 ? 0 : idx);
    form->addRow(QStringLiteral("状态"), box);
    return box;
}

void setupTable(QTableWidget *table) {
    TableStyle::applyDataTable(table);
}

void setScopeChecks(const QString &scope, QCheckBox *search, QCheckBox *detail, QCheckBox *qa) {
    const QStringList parts = scope.isEmpty()
                                  ? QStringList{QStringLiteral("search"), QStringLiteral("detail")}
                                  : scope.split(',', Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QString s = part.trimmed().toLower();
        if (s == "search") {
            search->setChecked(true);
        } else if (s == "detail") {
            detail->setChecked(true);
        } else if (s == "qa") {
            qa->setChecked(true);
        }
    }
}

QString scopeText(QCheckBox *search, QCheckBox *detail, QCheckBox *qa) {
    QStringList scopes;
    if (search->isChecked()) {
        scopes << QStringLiteral("search");
    }
    if (detail->isChecked()) {
        scopes << QStringLiteral("detail");
    }
    if (qa->isChecked()) {
        scopes << QStringLiteral("qa");
    }
    return scopes.join(',');
}

bool runAppDialog(QWidget *parent, const QString &title, const QJsonObject &initial,
                  bool editing, QJsonObject *out) {
    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setMinimumWidth(520);

    auto *layout = new QVBoxLayout(&dlg);
    auto *form = new QFormLayout();
    auto *name = line(form, QStringLiteral("应用名称"), initial.value("appName").toString());
    auto *appKey = line(form, QStringLiteral("AppKey"), initial.value("appKey").toString());
    appKey->setPlaceholderText(QStringLiteral("留空自动生成"));

    auto *appSecret = line(form, QStringLiteral("AppSecret"), QString());
    appSecret->setPlaceholderText(editing ? QStringLiteral("留空保持不变")
                                          : QStringLiteral("留空自动生成"));

    auto *status = statusBox(form, initial.value("status").toString(QStringLiteral("ENABLED")));

    auto *rateLimit = new QSpinBox();
    rateLimit->setRange(1, 100000);
    rateLimit->setValue(initial.value("rateLimit").toInt(60));
    form->addRow(QStringLiteral("每分钟限流"), rateLimit);

    auto *scopeRow = new QWidget();
    auto *scopeLayout = new QHBoxLayout(scopeRow);
    scopeLayout->setContentsMargins(0, 0, 0, 0);
    auto *search = new QCheckBox(QStringLiteral("检索"));
    auto *detail = new QCheckBox(QStringLiteral("详情"));
    auto *qa = new QCheckBox(QStringLiteral("问答"));
    scopeLayout->addWidget(search);
    scopeLayout->addWidget(detail);
    scopeLayout->addWidget(qa);
    scopeLayout->addStretch();
    setScopeChecks(initial.value("scope").toString(QStringLiteral("search,detail")), search, detail, qa);
    form->addRow(QStringLiteral("授权范围"), scopeRow);

    auto *remark = new QTextEdit();
    remark->setFixedHeight(80);
    remark->setText(initial.value("remark").toString());
    form->addRow(QStringLiteral("备注"), remark);

    layout->addLayout(form);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) {
        return false;
    }
    if (name->text().trimmed().isEmpty()) {
        notify::warn(parent, QStringLiteral("应用名称不能为空"));
        return false;
    }
    const QString scopes = scopeText(search, detail, qa);
    if (scopes.isEmpty()) {
        notify::warn(parent, QStringLiteral("请至少选择一个授权范围"));
        return false;
    }

    (*out)["appName"] = name->text().trimmed();
    if (!appKey->text().trimmed().isEmpty()) {
        (*out)["appKey"] = appKey->text().trimmed();
    }
    if (!appSecret->text().trimmed().isEmpty()) {
        (*out)["appSecret"] = appSecret->text().trimmed();
    }
    (*out)["status"] = status->currentText();
    (*out)["rateLimit"] = rateLimit->value();
    (*out)["scope"] = scopes;
    (*out)["remark"] = remark->toPlainText().trimmed();
    return true;
}

} // namespace

OpenApiPage::OpenApiPage(const QString &title, QWidget *parent)
    : QWidget(parent), m_title(title) {
    buildUi();
    refresh();
}

void OpenApiPage::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 24);
    root->setSpacing(14);

    auto *toolbar = new QHBoxLayout();
    m_keyword = new QLineEdit(this);
    m_keyword->setPlaceholderText(QStringLiteral("应用名称 / AppKey"));
    m_keyword->setMinimumWidth(220);

    m_statusFilter = new QComboBox(this);
    m_statusFilter->addItem(QStringLiteral("全部状态"), QString());
    m_statusFilter->addItem(QStringLiteral("ENABLED"), QStringLiteral("ENABLED"));
    m_statusFilter->addItem(QStringLiteral("DISABLED"), QStringLiteral("DISABLED"));

    auto *query = new QPushButton(QStringLiteral("查询"), this);
    m_add = new QPushButton(QStringLiteral("新增"), this);
    m_edit = new QPushButton(QStringLiteral("编辑"), this);
    m_delete = new QPushButton(QStringLiteral("删除"), this);
    m_resetSecret = new QPushButton(QStringLiteral("重置密钥"), this);
    m_add->setObjectName("PrimaryButton");
    for (QPushButton *button : {query, m_edit, m_delete, m_resetSecret}) {
        button->setObjectName("GhostButton");
    }

    toolbar->addWidget(m_keyword);
    toolbar->addWidget(m_statusFilter);
    toolbar->addWidget(query);
    toolbar->addSpacing(12);
    toolbar->addWidget(m_add);
    toolbar->addWidget(m_edit);
    toolbar->addWidget(m_delete);
    toolbar->addWidget(m_resetSecret);
    toolbar->addStretch();
    root->addLayout(toolbar);

    m_status = new QLabel(this);
    m_status->setObjectName("StatusLabel");
    root->addWidget(m_status);

    m_table = new QTableWidget(this);
    setupTable(m_table);
    m_table->setColumnCount(9);
    m_table->setHorizontalHeaderLabels({QStringLiteral("ID"), QStringLiteral("应用名称"),
                                        QStringLiteral("AppKey"), QStringLiteral("AppSecret"),
                                        QStringLiteral("状态"), QStringLiteral("限流/分钟"),
                                        QStringLiteral("授权范围"), QStringLiteral("备注"),
                                        QStringLiteral("创建时间")});
    TableStyle::configureTitleTable(m_table, 1);
    root->addWidget(m_table, 1);

    m_add->setEnabled(Session::instance().hasPermission(QStringLiteral("openapi:app:create")));
    m_edit->setEnabled(Session::instance().hasPermission(QStringLiteral("openapi:app:update")));
    m_resetSecret->setEnabled(Session::instance().hasPermission(QStringLiteral("openapi:app:update")));
    m_delete->setEnabled(Session::instance().hasPermission(QStringLiteral("openapi:app:delete")));

    connect(query, &QPushButton::clicked, this, &OpenApiPage::refresh);
    connect(m_keyword, &QLineEdit::returnPressed, this, &OpenApiPage::refresh);
    connect(m_statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this]() { refresh(); });
    connect(m_add, &QPushButton::clicked, this, &OpenApiPage::createApp);
    connect(m_edit, &QPushButton::clicked, this, &OpenApiPage::editApp);
    connect(m_delete, &QPushButton::clicked, this, &OpenApiPage::deleteApp);
    connect(m_resetSecret, &QPushButton::clicked, this, &OpenApiPage::resetSecret);
    connect(m_table, &QTableWidget::doubleClicked, this, &OpenApiPage::editApp);
}

QString OpenApiPage::buildListPath() const {
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("page"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("pageSize"), QStringLiteral("100"));
    const QString keyword = m_keyword ? m_keyword->text().trimmed() : QString();
    if (!keyword.isEmpty()) {
        query.addQueryItem(QStringLiteral("keyword"), keyword);
    }
    const QString status = m_statusFilter ? m_statusFilter->currentData().toString() : QString();
    if (!status.isEmpty()) {
        query.addQueryItem(QStringLiteral("status"), status);
    }
    return QStringLiteral("/openapi/app?") + query.toString(QUrl::FullyEncoded);
}

void OpenApiPage::refreshPage() {
    refresh();
}

void OpenApiPage::refresh() {
    setStatus(QStringLiteral("加载中..."));
    ApiClient::instance().get(buildListPath(), [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        fillTable(r.object());
    });
}

void OpenApiPage::fillTable(const QJsonObject &pageData) {
    const QJsonArray list = pageData.value("list").toArray();
    m_table->setRowCount(0);
    for (const QJsonValue &v : list) {
        const QJsonObject o = v.toObject();
        const int row = m_table->rowCount();
        m_table->insertRow(row);

        QStringList values{textOf(o, "id"),
                           o.value("appName").toString(),
                           o.value("appKey").toString(),
                           o.value("appSecretMasked").toString(),
                           o.value("status").toString(),
                           textOf(o, "rateLimit"),
                           o.value("scope").toString(),
                           o.value("remark").toString(),
                           timeText(o, "createTime")};
        for (int col = 0; col < values.size(); ++col) {
            auto *item = new QTableWidgetItem(values.at(col));
            if (col == 0) {
                item->setData(Qt::UserRole, o.value("id").toVariant());
            }
            m_table->setItem(row, col, item);
        }
    }
    TableStyle::setItemTooltipFromText(m_table);
    setStatus(QStringLiteral("已加载 %1 条").arg(list.size()));
}

void OpenApiPage::createApp() {
    QJsonObject body;
    if (!runAppDialog(this, QStringLiteral("新增开放应用"), {}, false, &body)) {
        return;
    }
    ApiClient::instance().post("/openapi/app", body, [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        const QJsonObject app = r.object();
        const QString secret = app.value("appSecret").toString();
        refresh();
        if (!secret.isEmpty()) {
            notify::info(this, QStringLiteral("应用已创建。\nAppKey：%1\nAppSecret：%2\n请妥善保存，后续只能重置。")
                                   .arg(app.value("appKey").toString(), secret));
        } else {
            setStatus(QStringLiteral("应用已创建"));
        }
    });
}

void OpenApiPage::editApp() {
    const qint64 id = selectedId();
    if (id <= 0) {
        setStatus(QStringLiteral("请先选择一条开放应用"), true);
        return;
    }
    const QString path = QStringLiteral("/openapi/app/%1").arg(id);
    ApiClient::instance().get(path, [this, path](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        QJsonObject body;
        if (!runAppDialog(this, QStringLiteral("编辑开放应用"), r.object(), true, &body)) {
            return;
        }
        ApiClient::instance().put(path, body, [this](const ApiResponse &wr) {
            wr.ok ? refresh() : setStatus(wr.message, true);
        });
    });
}

void OpenApiPage::deleteApp() {
    const qint64 id = selectedId();
    if (id <= 0) {
        setStatus(QStringLiteral("请先选择一条开放应用"), true);
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("确认删除"),
                              QStringLiteral("确认删除开放应用“%1”？").arg(selectedName()))
        != QMessageBox::Yes) {
        return;
    }
    ApiClient::instance().del(QStringLiteral("/openapi/app/%1").arg(id), [this](const ApiResponse &r) {
        r.ok ? refresh() : setStatus(r.message, true);
    });
}

void OpenApiPage::resetSecret() {
    const qint64 id = selectedId();
    if (id <= 0) {
        setStatus(QStringLiteral("请先选择一条开放应用"), true);
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("重置密钥"),
                              QStringLiteral("确认重置“%1”的 AppSecret？旧密钥会立即失效。").arg(selectedName()))
        != QMessageBox::Yes) {
        return;
    }
    ApiClient::instance().put(QStringLiteral("/openapi/app/%1/secret").arg(id), {}, [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        const QString secret = r.object().value("appSecret").toString();
        refresh();
        notify::info(this, QStringLiteral("新 AppSecret：%1\n请妥善保存，关闭后不再明文展示。").arg(secret));
    });
}

qint64 OpenApiPage::selectedId() const {
    if (!m_table || m_table->currentRow() < 0 || !m_table->item(m_table->currentRow(), 0)) {
        return 0;
    }
    return m_table->item(m_table->currentRow(), 0)->data(Qt::UserRole).toLongLong();
}

QString OpenApiPage::selectedName() const {
    if (!m_table || m_table->currentRow() < 0 || !m_table->item(m_table->currentRow(), 1)) {
        return QString();
    }
    return m_table->item(m_table->currentRow(), 1)->text();
}

void OpenApiPage::setStatus(const QString &text, bool error) {
    if (!m_status) {
        return;
    }
    m_status->setText(text);
    m_status->setStyleSheet(error ? "color:#DC2626;" : "color:#6B7280;");
    if (error && !text.isEmpty()) {
        notify::warn(this, text);
    }
}

} // namespace kb
