#include "app/SystemAdminPage.h"

#include "app/RefreshablePage.h"
#include "common/TableStyle.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>

#include <functional>

namespace kb {

namespace {

QString textOf(const QJsonObject &o, const QString &key) {
    const QJsonValue v = o.value(key);
    if (v.isDouble()) {
        return QString::number(static_cast<qint64>(v.toDouble()));
    }
    return v.toString();
}

QJsonArray idArrayFromCsv(const QString &text) {
    QJsonArray arr;
    for (const QString &part : text.split(',', Qt::SkipEmptyParts)) {
        bool ok = false;
        qint64 id = part.trimmed().toLongLong(&ok);
        if (ok) {
            arr.append(id);
        }
    }
    return arr;
}

QString csvFromArray(const QJsonArray &arr) {
    QStringList parts;
    for (const QJsonValue &v : arr) {
        parts.append(QString::number(static_cast<qint64>(v.toDouble())));
    }
    return parts.join(',');
}

QString idsText(const QJsonObject &o, const QString &key) {
    return csvFromArray(o.value(key).toArray());
}

QTableWidget *makeTable(QWidget *parent) {
    auto *table = new QTableWidget(parent);
    TableStyle::applyDataTable(table);
    return table;
}

QLineEdit *line(QFormLayout *form, const QString &label, const QString &value = QString()) {
    auto *edit = new QLineEdit();
    edit->setText(value);
    form->addRow(label, edit);
    return edit;
}

QComboBox *statusBox(QFormLayout *form, const QString &value = QStringLiteral("ENABLED")) {
    auto *box = new QComboBox();
    box->addItems({QStringLiteral("ENABLED"), QStringLiteral("DISABLED")});
    int idx = box->findText(value);
    box->setCurrentIndex(idx < 0 ? 0 : idx);
    form->addRow(QStringLiteral("状态"), box);
    return box;
}

bool runUserDialog(QWidget *parent, const QString &title, const QJsonObject &initial, QJsonObject *out) {
    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    auto *layout = new QVBoxLayout(&dlg);
    auto *form = new QFormLayout();
    auto *orgId = line(form, QStringLiteral("机构ID"), textOf(initial, "orgId"));
    auto *username = line(form, QStringLiteral("用户名"), initial.value("username").toString());
    auto *password = line(form, QStringLiteral("密码"), QString());
    password->setPlaceholderText(QStringLiteral("新增为空默认 123456；编辑请用改密"));
    auto *realName = line(form, QStringLiteral("姓名"), initial.value("realName").toString());
    auto *phone = line(form, QStringLiteral("手机"), initial.value("phone").toString());
    auto *email = line(form, QStringLiteral("邮箱"), initial.value("email").toString());
    auto *gender = line(form, QStringLiteral("性别"), initial.value("gender").toString("U"));
    auto *status = statusBox(form, initial.value("status").toString("ENABLED"));
    auto *roles = line(form, QStringLiteral("角色ID"), idsText(initial, "roleIds"));
    layout->addLayout(form);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) {
        return false;
    }
    (*out)["orgId"] = orgId->text().trimmed().isEmpty() ? QJsonValue() : orgId->text().trimmed().toLongLong();
    (*out)["username"] = username->text().trimmed();
    if (!password->text().isEmpty()) {
        (*out)["password"] = password->text();
    }
    (*out)["realName"] = realName->text().trimmed();
    (*out)["phone"] = phone->text().trimmed();
    (*out)["email"] = email->text().trimmed();
    (*out)["gender"] = gender->text().trimmed().isEmpty() ? QStringLiteral("U") : gender->text().trimmed();
    (*out)["status"] = status->currentText();
    (*out)["roleIds"] = idArrayFromCsv(roles->text());
    return true;
}

bool runRoleDialog(QWidget *parent, const QString &title, const QJsonObject &initial, QJsonObject *out) {
    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    auto *layout = new QVBoxLayout(&dlg);
    auto *form = new QFormLayout();
    auto *name = line(form, QStringLiteral("角色名称"), initial.value("name").toString());
    auto *code = line(form, QStringLiteral("角色编码"), initial.value("code").toString());
    auto *dataScope = line(form, QStringLiteral("数据权限"), initial.value("dataScope").toString("SELF"));
    auto *sort = new QSpinBox();
    sort->setRange(0, 9999);
    sort->setValue(initial.value("sort").toInt(0));
    form->addRow(QStringLiteral("排序"), sort);
    auto *status = statusBox(form, initial.value("status").toString("ENABLED"));
    auto *remark = line(form, QStringLiteral("备注"), initial.value("remark").toString());
    layout->addLayout(form);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) {
        return false;
    }
    (*out)["name"] = name->text().trimmed();
    (*out)["code"] = code->text().trimmed();
    (*out)["dataScope"] = dataScope->text().trimmed().isEmpty() ? QStringLiteral("SELF") : dataScope->text().trimmed();
    (*out)["sort"] = sort->value();
    (*out)["status"] = status->currentText();
    (*out)["remark"] = remark->text().trimmed();
    return true;
}

bool runPermissionDialog(QWidget *parent, const QString &title, const QJsonObject &initial, QJsonObject *out) {
    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    auto *layout = new QVBoxLayout(&dlg);
    auto *form = new QFormLayout();
    auto *parentId = line(form, QStringLiteral("上级ID"), textOf(initial, "parentId"));
    auto *name = line(form, QStringLiteral("名称"), initial.value("name").toString());
    auto *type = new QComboBox();
    type->addItems({QStringLiteral("DIR"), QStringLiteral("MENU"), QStringLiteral("BUTTON")});
    int idx = type->findText(initial.value("type").toString("BUTTON"));
    type->setCurrentIndex(idx < 0 ? 2 : idx);
    form->addRow(QStringLiteral("类型"), type);
    auto *code = line(form, QStringLiteral("权限码"), initial.value("permissionCode").toString());
    auto *route = line(form, QStringLiteral("路由名"), initial.value("route").toString());
    auto *icon = line(form, QStringLiteral("图标"), initial.value("icon").toString());
    auto *sort = new QSpinBox();
    sort->setRange(0, 9999);
    sort->setValue(initial.value("sort").toInt(0));
    form->addRow(QStringLiteral("排序"), sort);
    auto *status = statusBox(form, initial.value("status").toString("ENABLED"));
    layout->addLayout(form);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) {
        return false;
    }
    (*out)["parentId"] = parentId->text().trimmed().isEmpty() ? 0 : parentId->text().trimmed().toLongLong();
    (*out)["name"] = name->text().trimmed();
    (*out)["type"] = type->currentText();
    (*out)["permissionCode"] = code->text().trimmed();
    (*out)["route"] = route->text().trimmed();
    (*out)["icon"] = icon->text().trimmed();
    (*out)["sort"] = sort->value();
    (*out)["status"] = status->currentText();
    return true;
}

void setHeaders(QTableWidget *table, const QStringList &headers) {
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setRowCount(0);
}

} // namespace

SystemAdminPage::SystemAdminPage(const QString &title, const QString &routeName, QWidget *parent)
    : QWidget(parent), m_title(title), m_routeName(routeName) {
    buildUi();
    refresh();
}

void SystemAdminPage::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 24);
    root->setSpacing(14);

    auto *toolbar = new QHBoxLayout();
    m_add = new QPushButton(QStringLiteral("新增"), this);
    m_edit = new QPushButton(QStringLiteral("编辑"), this);
    m_delete = new QPushButton(QStringLiteral("删除"), this);
    m_toggleStatus = new QPushButton(QStringLiteral("启停"), this);
    m_resetPassword = new QPushButton(QStringLiteral("改密"), this);
    m_assignPermissions = new QPushButton(QStringLiteral("分配权限"), this);
    m_add->setObjectName("PrimaryButton");
    for (QPushButton *button : {m_edit, m_delete, m_toggleStatus, m_resetPassword, m_assignPermissions}) {
        button->setObjectName("GhostButton");
    }
    toolbar->addWidget(m_add);
    toolbar->addWidget(m_edit);
    toolbar->addWidget(m_delete);
    toolbar->addWidget(m_toggleStatus);
    toolbar->addWidget(m_resetPassword);
    toolbar->addWidget(m_assignPermissions);
    toolbar->addStretch();
    root->addLayout(toolbar);

    m_status = new QLabel(this);
    m_status->setObjectName("StatusLabel");
    root->addWidget(m_status);

    if (m_routeName == "system.log") {
        m_logTabs = new QTabWidget(this);
        m_loginLogTable = makeTable(m_logTabs);
        m_operationLogTable = makeTable(m_logTabs);
        m_logTabs->addTab(m_loginLogTable, QStringLiteral("登录日志"));
        m_logTabs->addTab(m_operationLogTable, QStringLiteral("操作日志"));
        root->addWidget(m_logTabs, 1);
        m_add->hide();
        m_edit->hide();
        m_delete->hide();
        m_toggleStatus->hide();
        m_resetPassword->hide();
        m_assignPermissions->hide();
    } else {
        m_table = makeTable(this);
        root->addWidget(m_table, 1);
        m_toggleStatus->setVisible(m_routeName == "system.user");
        m_resetPassword->setVisible(m_routeName == "system.user");
        m_assignPermissions->setVisible(m_routeName == "system.role");
    }

    connect(m_add, &QPushButton::clicked, this, &SystemAdminPage::createItem);
    connect(m_edit, &QPushButton::clicked, this, &SystemAdminPage::editItem);
    connect(m_delete, &QPushButton::clicked, this, &SystemAdminPage::deleteItem);
    connect(m_toggleStatus, &QPushButton::clicked, this, &SystemAdminPage::toggleUserStatus);
    connect(m_resetPassword, &QPushButton::clicked, this, &SystemAdminPage::resetUserPassword);
    connect(m_assignPermissions, &QPushButton::clicked, this, &SystemAdminPage::assignRolePermissions);
}

void SystemAdminPage::refreshPage() {
    refresh();
}

void SystemAdminPage::refresh() {
    setStatus(QStringLiteral("加载中..."));
    if (m_routeName == "system.user") {
        refreshUsers();
    } else if (m_routeName == "system.role") {
        refreshRoles();
    } else if (m_routeName == "system.permission") {
        refreshPermissions();
    } else if (m_routeName == "system.log") {
        refreshLogs();
    }
}

void SystemAdminPage::refreshUsers() {
    ApiClient::instance().get("/system/user?page=1&pageSize=100", [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        fillPageTable(r.object(), {QStringLiteral("ID"), QStringLiteral("用户名"), QStringLiteral("姓名"),
                                   QStringLiteral("机构"), QStringLiteral("手机"), QStringLiteral("状态"),
                                   QStringLiteral("角色ID")},
                      [](const QJsonObject &o) {
                          return QStringList{textOf(o, "id"), o.value("username").toString(),
                                             o.value("realName").toString(), textOf(o, "orgId"),
                                             o.value("phone").toString(), o.value("status").toString(),
                                             idsText(o, "roleIds")};
                      });
    });
}

void SystemAdminPage::refreshRoles() {
    ApiClient::instance().get("/system/role?page=1&pageSize=100", [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        fillPageTable(r.object(), {QStringLiteral("ID"), QStringLiteral("名称"), QStringLiteral("编码"),
                                   QStringLiteral("数据权限"), QStringLiteral("排序"), QStringLiteral("状态"),
                                   QStringLiteral("权限ID")},
                      [](const QJsonObject &o) {
                          return QStringList{textOf(o, "id"), o.value("name").toString(),
                                             o.value("code").toString(), o.value("dataScope").toString(),
                                             textOf(o, "sort"), o.value("status").toString(),
                                             idsText(o, "permissionIds")};
                      });
    });
}

void SystemAdminPage::refreshPermissions() {
    ApiClient::instance().get("/system/permission/tree", [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        setHeaders(m_table, {QStringLiteral("ID"), QStringLiteral("名称"), QStringLiteral("类型"),
                             QStringLiteral("权限码"), QStringLiteral("路由"), QStringLiteral("排序"),
                             QStringLiteral("状态")});
        fillPermissionRows(r.data.toArray(), 0);
        TableStyle::configureTitleTable(m_table, 1);
        TableStyle::setItemTooltipFromText(m_table);
        setStatus(QStringLiteral("已加载 %1 条").arg(m_table->rowCount()));
    });
}

void SystemAdminPage::refreshLogs() {
    ApiClient::instance().get("/system/log/login?page=1&pageSize=100", [this](const ApiResponse &r) {
        if (r.ok) {
            fillLogTable(m_loginLogTable, r.object(), {QStringLiteral("ID"), QStringLiteral("用户"),
                                                       QStringLiteral("IP"), QStringLiteral("状态"),
                                                       QStringLiteral("消息"), QStringLiteral("时间")},
                         [](const QJsonObject &o) {
                             return QStringList{textOf(o, "id"), o.value("username").toString(),
                                                o.value("loginIp").toString(), o.value("status").toString(),
                                                o.value("msg").toString(), o.value("loginTime").toString()};
                         });
        }
    });
    ApiClient::instance().get("/system/log/operation?page=1&pageSize=100", [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        fillLogTable(m_operationLogTable, r.object(), {QStringLiteral("ID"), QStringLiteral("用户"),
                                                       QStringLiteral("模块"), QStringLiteral("操作"),
                                                       QStringLiteral("状态"), QStringLiteral("耗时"),
                                                       QStringLiteral("时间")},
                     [](const QJsonObject &o) {
                         return QStringList{textOf(o, "id"), o.value("username").toString(),
                                            o.value("module").toString(), o.value("operation").toString(),
                                            o.value("status").toString(), textOf(o, "costTime"),
                                            o.value("operationTime").toString()};
                     });
        setStatus(QStringLiteral("日志已刷新"));
    });
}

void SystemAdminPage::fillPageTable(const QJsonObject &pageData, const QStringList &headers,
                                    const std::function<QStringList(const QJsonObject &)> &rowMapper) {
    setHeaders(m_table, headers);
    const QJsonArray list = pageData.value("list").toArray();
    for (const QJsonValue &v : list) {
        QJsonObject o = v.toObject();
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        const QStringList values = rowMapper(o);
        for (int col = 0; col < values.size(); ++col) {
            auto *item = new QTableWidgetItem(values.at(col));
            if (col == 0) {
                item->setData(Qt::UserRole, o.value("id").toVariant());
            }
            m_table->setItem(row, col, item);
        }
    }
    int stretchColumn = 1;
    if (m_routeName == QStringLiteral("system.user")) {
        stretchColumn = 2; // 姓名
    }
    TableStyle::configureTitleTable(m_table, stretchColumn);
    TableStyle::setItemTooltipFromText(m_table);
    setStatus(QStringLiteral("已加载 %1 条").arg(list.size()));
}

void SystemAdminPage::fillPermissionRows(const QJsonArray &items, int depth) {
    for (const QJsonValue &v : items) {
        QJsonObject o = v.toObject();
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        QStringList values{textOf(o, "id"), QString(depth * 2, ' ') + o.value("name").toString(),
                           o.value("type").toString(), o.value("permissionCode").toString(),
                           o.value("route").toString(), textOf(o, "sort"), o.value("status").toString()};
        for (int col = 0; col < values.size(); ++col) {
            auto *item = new QTableWidgetItem(values.at(col));
            if (col == 0) {
                item->setData(Qt::UserRole, o.value("id").toVariant());
            }
            m_table->setItem(row, col, item);
        }
        fillPermissionRows(o.value("children").toArray(), depth + 1);
    }
}

void SystemAdminPage::fillLogTable(QTableWidget *table, const QJsonObject &pageData, const QStringList &headers,
                                   const std::function<QStringList(const QJsonObject &)> &rowMapper) {
    setHeaders(table, headers);
    const QJsonArray list = pageData.value("list").toArray();
    for (const QJsonValue &v : list) {
        QJsonObject o = v.toObject();
        const int row = table->rowCount();
        table->insertRow(row);
        const QStringList values = rowMapper(o);
        for (int col = 0; col < values.size(); ++col) {
            table->setItem(row, col, new QTableWidgetItem(values.at(col)));
        }
    }
    TableStyle::configureTitleTable(table, headers.size() - 1);
    TableStyle::setItemTooltipFromText(table);
}

void SystemAdminPage::createItem() {
    QJsonObject body;
    if (m_routeName == "system.user" && runUserDialog(this, QStringLiteral("新增人员"), {}, &body)) {
        ApiClient::instance().post("/system/user", body, [this](const ApiResponse &r) {
            r.ok ? refresh() : setStatus(r.message, true);
        });
    } else if (m_routeName == "system.role" && runRoleDialog(this, QStringLiteral("新增角色"), {}, &body)) {
        ApiClient::instance().post("/system/role", body, [this](const ApiResponse &r) {
            r.ok ? refresh() : setStatus(r.message, true);
        });
    } else if (m_routeName == "system.permission" && runPermissionDialog(this, QStringLiteral("新增权限"), {}, &body)) {
        ApiClient::instance().post("/system/permission", body, [this](const ApiResponse &r) {
            r.ok ? refresh() : setStatus(r.message, true);
        });
    }
}

void SystemAdminPage::editItem() {
    qint64 id = selectedId();
    if (id <= 0) {
        setStatus(QStringLiteral("请先选择一行"), true);
        return;
    }
    QString path = m_routeName == "system.user" ? "/system/user/" + QString::number(id)
                 : m_routeName == "system.role" ? "/system/role/" + QString::number(id)
                 : "/system/permission/" + QString::number(id);
    ApiClient::instance().get(path, [this, id, path](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        QJsonObject body;
        bool accepted = false;
        if (m_routeName == "system.user") {
            accepted = runUserDialog(this, QStringLiteral("编辑人员"), r.object(), &body);
        } else if (m_routeName == "system.role") {
            accepted = runRoleDialog(this, QStringLiteral("编辑角色"), r.object(), &body);
        } else {
            accepted = runPermissionDialog(this, QStringLiteral("编辑权限"), r.object(), &body);
        }
        if (!accepted) {
            return;
        }
        ApiClient::instance().put(path, body, [this](const ApiResponse &wr) {
            wr.ok ? refresh() : setStatus(wr.message, true);
        });
    });
}

void SystemAdminPage::deleteItem() {
    qint64 id = selectedId();
    if (id <= 0) {
        setStatus(QStringLiteral("请先选择一行"), true);
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("确认删除"), QStringLiteral("确认删除选中记录？"))
        != QMessageBox::Yes) {
        return;
    }
    QString path = m_routeName == "system.user" ? "/system/user/" + QString::number(id)
                 : m_routeName == "system.role" ? "/system/role/" + QString::number(id)
                 : "/system/permission/" + QString::number(id);
    ApiClient::instance().del(path, [this](const ApiResponse &r) {
        r.ok ? refresh() : setStatus(r.message, true);
    });
}

void SystemAdminPage::toggleUserStatus() {
    qint64 id = selectedId();
    if (id <= 0) {
        setStatus(QStringLiteral("请先选择一行"), true);
        return;
    }
    QString status = QInputDialog::getItem(this, QStringLiteral("启停人员"), QStringLiteral("状态"),
                                           {QStringLiteral("ENABLED"), QStringLiteral("DISABLED")}, 0, false);
    if (status.isEmpty()) {
        return;
    }
    ApiClient::instance().put("/system/user/" + QString::number(id) + "/status",
                              QJsonObject{{"status", status}},
                              [this](const ApiResponse &r) { r.ok ? refresh() : setStatus(r.message, true); });
}

void SystemAdminPage::resetUserPassword() {
    qint64 id = selectedId();
    if (id <= 0) {
        setStatus(QStringLiteral("请先选择一行"), true);
        return;
    }
    bool ok = false;
    QString password = QInputDialog::getText(this, QStringLiteral("修改密码"), QStringLiteral("新密码"),
                                             QLineEdit::Password, QString(), &ok);
    if (!ok || password.isEmpty()) {
        return;
    }
    ApiClient::instance().put("/system/user/" + QString::number(id) + "/password",
                              QJsonObject{{"password", password}},
                              [this](const ApiResponse &r) { r.ok ? setStatus(QStringLiteral("密码已更新")) : setStatus(r.message, true); });
}

void SystemAdminPage::assignRolePermissions() {
    qint64 id = selectedId();
    if (id <= 0) {
        setStatus(QStringLiteral("请先选择一行"), true);
        return;
    }
    ApiClient::instance().get("/system/role/" + QString::number(id), [this, id](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        bool ok = false;
        QString text = QInputDialog::getText(this, QStringLiteral("分配权限"),
                                             QStringLiteral("权限ID，逗号分隔"),
                                             QLineEdit::Normal, idsText(r.object(), "permissionIds"), &ok);
        if (!ok) {
            return;
        }
        ApiClient::instance().put("/system/role/" + QString::number(id) + "/permissions",
                                  QJsonObject{{"permissionIds", idArrayFromCsv(text)}},
                                  [this](const ApiResponse &wr) { wr.ok ? refresh() : setStatus(wr.message, true); });
    });
}

qint64 SystemAdminPage::selectedId() const {
    if (!m_table || m_table->currentRow() < 0 || !m_table->item(m_table->currentRow(), 0)) {
        return 0;
    }
    return m_table->item(m_table->currentRow(), 0)->data(Qt::UserRole).toLongLong();
}

void SystemAdminPage::setStatus(const QString &text, bool error) {
    if (!m_status) {
        return;
    }
    m_status->setText(text);
    m_status->setStyleSheet(error ? "color:#B94A48;" : "color:#757575;");
    if (error && !text.isEmpty()) {
        notify::warn(this, text);
    }
}

} // namespace kb
