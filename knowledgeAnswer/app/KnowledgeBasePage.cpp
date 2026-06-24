#include "app/KnowledgeBasePage.h"

#include "app/RefreshablePage.h"
#include "common/TableStyle.h"
#include "core/auth/Session.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QAbstractItemView>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
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

void styleTagColorPreview(QFrame *preview, const QString &hex) {
    preview->setObjectName(QStringLiteral("TagColorPreview"));
    const QColor c(hex);
    if (c.isValid()) {
        preview->setStyleSheet(QStringLiteral(
            "QFrame#TagColorPreview { background-color: %1; border: 1px solid #E0D9CE; border-radius: 6px; }")
                                   .arg(c.name(QColor::HexRgb)));
    } else {
        preview->setStyleSheet(QStringLiteral(
            "QFrame#TagColorPreview { background-color: #FEFEFE; border: 1px dashed #E0D9CE; border-radius: 6px; }"));
    }
}

QWidget *makeTagColorCell(const QString &hex, QWidget *parent) {
    auto *cell = new QWidget(parent);
    auto *layout = new QHBoxLayout(cell);
    layout->setContentsMargins(4, 2, 8, 2);
    layout->setSpacing(10);
    auto *preview = new QFrame(cell);
    preview->setFixedSize(28, 28);
    styleTagColorPreview(preview, hex);
    auto *text = new QLabel(hex.isEmpty() ? QStringLiteral("—") : hex, cell);
    text->setObjectName(QStringLiteral("MutedLabel"));
    text->setMinimumWidth(72);
    layout->addWidget(preview);
    layout->addWidget(text, 1);
    return cell;
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
    int idx = box->findText(value);
    box->setCurrentIndex(idx < 0 ? 0 : idx);
    form->addRow(QStringLiteral("状态"), box);
    return box;
}

// 分类弹窗：上级只读展示（不在此处迁移层级），返回 name/code/sort/status。
bool runCategoryDialog(QWidget *parent, const QString &title, const QString &parentLabel,
                       const QJsonObject &initial, QJsonObject *out) {
    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setMinimumWidth(360);
    auto *layout = new QVBoxLayout(&dlg);
    auto *form = new QFormLayout();
    auto *parentField = line(form, QStringLiteral("上级"), parentLabel);
    parentField->setEnabled(false);
    auto *name = line(form, QStringLiteral("名称"), initial.value("name").toString());
    auto *code = line(form, QStringLiteral("编码"), initial.value("code").toString());
    auto *sort = new QSpinBox();
    sort->setRange(0, 9999);
    sort->setValue(initial.value("sort").toInt(0));
    form->addRow(QStringLiteral("排序"), sort);
    auto *status = statusBox(form, initial.value("status").toString(QStringLiteral("ENABLED")));
    layout->addLayout(form);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) {
        return false;
    }
    if (name->text().trimmed().isEmpty()) {
        notify::warn(parent, QStringLiteral("分类名称不能为空"));
        return false;
    }
    (*out)["name"] = name->text().trimmed();
    (*out)["code"] = code->text().trimmed();
    (*out)["sort"] = sort->value();
    (*out)["status"] = status->currentText();
    return true;
}

// 标签弹窗：颜色用 QLineEdit(#RRGGBB) + 色块预览 + 取色按钮（QColorDialog）。
bool runTagDialog(QWidget *parent, const QString &title, const QJsonObject &initial, QJsonObject *out) {
    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setMinimumWidth(360);
    auto *layout = new QVBoxLayout(&dlg);
    auto *form = new QFormLayout();
    auto *name = line(form, QStringLiteral("名称"), initial.value("name").toString());

    auto *colorRow = new QWidget();
    auto *colorLayout = new QHBoxLayout(colorRow);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    colorLayout->setSpacing(8);
    auto *colorEdit = new QLineEdit(initial.value("color").toString());
    colorEdit->setPlaceholderText(QStringLiteral("#RRGGBB"));
    auto *swatch = new QFrame();
    swatch->setFixedSize(28, 28);
    swatch->setFrameShape(QFrame::NoFrame);
    auto *pick = new QPushButton(QStringLiteral("取色..."));
    pick->setObjectName("GhostButton");
    colorLayout->addWidget(colorEdit, 1);
    colorLayout->addWidget(swatch);
    colorLayout->addWidget(pick);
    form->addRow(QStringLiteral("颜色"), colorRow);

    auto applySwatch = [swatch](const QString &hex) {
        styleTagColorPreview(swatch, hex);
    };
    applySwatch(colorEdit->text());
    QObject::connect(colorEdit, &QLineEdit::textChanged, swatch, applySwatch);
    QObject::connect(pick, &QPushButton::clicked, &dlg, [&dlg, colorEdit]() {
        QColor init(colorEdit->text().trimmed());
        QColor c = QColorDialog::getColor(init.isValid() ? init : QColor("#9AA69D"), &dlg,
                                          QStringLiteral("选择标签颜色"));
        if (c.isValid()) {
            colorEdit->setText(c.name().toUpper());
        }
    });

    auto *sort = new QSpinBox();
    sort->setRange(0, 9999);
    sort->setValue(initial.value("sort").toInt(0));
    form->addRow(QStringLiteral("排序"), sort);
    layout->addLayout(form);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) {
        return false;
    }
    if (name->text().trimmed().isEmpty()) {
        notify::warn(parent, QStringLiteral("标签名称不能为空"));
        return false;
    }
    (*out)["name"] = name->text().trimmed();
    (*out)["color"] = colorEdit->text().trimmed();
    (*out)["sort"] = sort->value();
    return true;
}

} // namespace

KnowledgeBasePage::KnowledgeBasePage(const QString &title, QWidget *parent)
    : QWidget(parent), m_title(title) {
    buildUi();
    refreshCategories();
    refreshTags();
}

void KnowledgeBasePage::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 24);
    root->setSpacing(14);

    auto *tabs = new QTabWidget(this);
    m_tabs = tabs;
    tabs->addTab(buildCategoryTab(), QStringLiteral("分类"));
    tabs->addTab(buildTagTab(), QStringLiteral("标签"));
    root->addWidget(tabs, 1);
}

void KnowledgeBasePage::refreshPage() {
    if (!m_tabs) {
        return;
    }
    if (m_tabs->currentIndex() == 0) {
        refreshCategories();
    } else {
        refreshTags();
    }
}

QWidget *KnowledgeBasePage::buildCategoryTab() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 12, 0, 0);
    layout->setSpacing(12);

    auto *toolbar = new QHBoxLayout();
    m_catAddRoot = new QPushButton(QStringLiteral("新增根"), page);
    m_catAddChild = new QPushButton(QStringLiteral("新增子"), page);
    m_catEdit = new QPushButton(QStringLiteral("编辑"), page);
    m_catDelete = new QPushButton(QStringLiteral("删除"), page);
    m_catAddRoot->setObjectName("PrimaryButton");
    for (QPushButton *button : {m_catAddChild, m_catEdit, m_catDelete}) {
        button->setObjectName("GhostButton");
    }
    toolbar->addWidget(m_catAddRoot);
    toolbar->addWidget(m_catAddChild);
    toolbar->addWidget(m_catEdit);
    toolbar->addWidget(m_catDelete);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    m_catStatus = new QLabel(page);
    m_catStatus->setObjectName("StatusLabel");
    layout->addWidget(m_catStatus);

    m_catTree = new QTreeWidget(page);
    m_catTree->setColumnCount(4);
    m_catTree->setHeaderLabels({QStringLiteral("名称"), QStringLiteral("编码"),
                                QStringLiteral("排序"), QStringLiteral("状态")});
    TableStyle::configureCategoryTree(m_catTree);
    layout->addWidget(m_catTree, 1);

    const bool canCreate = Session::instance().hasPermission(QStringLiteral("knowledge:category:create"));
    m_catAddRoot->setEnabled(canCreate);
    m_catAddChild->setEnabled(canCreate);
    m_catEdit->setEnabled(Session::instance().hasPermission(QStringLiteral("knowledge:category:update")));
    m_catDelete->setEnabled(Session::instance().hasPermission(QStringLiteral("knowledge:category:delete")));

    connect(m_catAddRoot, &QPushButton::clicked, this, [this]() { addCategory(false); });
    connect(m_catAddChild, &QPushButton::clicked, this, [this]() { addCategory(true); });
    connect(m_catEdit, &QPushButton::clicked, this, &KnowledgeBasePage::editCategory);
    connect(m_catDelete, &QPushButton::clicked, this, &KnowledgeBasePage::deleteCategory);
    return page;
}

QWidget *KnowledgeBasePage::buildTagTab() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 12, 0, 0);
    layout->setSpacing(12);

    auto *toolbar = new QHBoxLayout();
    m_tagAdd = new QPushButton(QStringLiteral("新增"), page);
    m_tagEdit = new QPushButton(QStringLiteral("编辑"), page);
    m_tagDelete = new QPushButton(QStringLiteral("删除"), page);
    m_tagAdd->setObjectName("PrimaryButton");
    for (QPushButton *button : {m_tagEdit, m_tagDelete}) {
        button->setObjectName("GhostButton");
    }
    toolbar->addWidget(m_tagAdd);
    toolbar->addWidget(m_tagEdit);
    toolbar->addWidget(m_tagDelete);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    m_tagStatus = new QLabel(page);
    m_tagStatus->setObjectName("StatusLabel");
    layout->addWidget(m_tagStatus);

    m_tagTable = new QTableWidget(page);
    m_tagTable->setColumnCount(3);
    m_tagTable->setHorizontalHeaderLabels({QStringLiteral("名称"), QStringLiteral("颜色"),
                                           QStringLiteral("排序")});
    TableStyle::configureTagTable(m_tagTable);
    layout->addWidget(m_tagTable, 1);

    m_tagAdd->setEnabled(Session::instance().hasPermission(QStringLiteral("knowledge:tag:create")));
    m_tagEdit->setEnabled(Session::instance().hasPermission(QStringLiteral("knowledge:tag:update")));
    m_tagDelete->setEnabled(Session::instance().hasPermission(QStringLiteral("knowledge:tag:delete")));

    connect(m_tagAdd, &QPushButton::clicked, this, &KnowledgeBasePage::addTag);
    connect(m_tagEdit, &QPushButton::clicked, this, &KnowledgeBasePage::editTag);
    connect(m_tagDelete, &QPushButton::clicked, this, &KnowledgeBasePage::deleteTag);
    return page;
}

void KnowledgeBasePage::refreshCategories() {
    setCatStatus(QStringLiteral("加载中..."));
    ApiClient::instance().get("/knowledge/category/tree", [this](const ApiResponse &r) {
        if (!r.ok) {
            setCatStatus(r.message, true);
            return;
        }
        m_catTree->clear();
        appendCategoryNodes(nullptr, r.data.toArray());
        m_catTree->expandAll();
        TableStyle::setTreeItemTooltipFromText(m_catTree);
        setCatStatus(QStringLiteral("分类已加载"));
    });
}

void KnowledgeBasePage::appendCategoryNodes(QTreeWidgetItem *parentItem, const QJsonArray &nodes) {
    for (const QJsonValue &v : nodes) {
        const QJsonObject o = v.toObject();
        auto *item = parentItem ? new QTreeWidgetItem(parentItem) : new QTreeWidgetItem(m_catTree);
        item->setText(0, o.value("name").toString());
        item->setText(1, o.value("code").toString());
        item->setText(2, textOf(o, "sort"));
        item->setText(3, o.value("status").toString());
        item->setData(0, Qt::UserRole, o.value("id").toVariant());
        item->setData(0, Qt::UserRole + 1, o.value("parentId").toVariant());
        appendCategoryNodes(item, o.value("children").toArray());
    }
}

void KnowledgeBasePage::addCategory(bool asChild) {
    QJsonObject body;
    qint64 parentId = 0;
    QString parentLabel = QStringLiteral("（根分类）");
    if (asChild) {
        parentId = selectedCategoryId();
        if (parentId <= 0) {
            setCatStatus(QStringLiteral("请先选择上级分类"), true);
            return;
        }
        parentLabel = selectedCategoryName();
    }
    if (!runCategoryDialog(this, asChild ? QStringLiteral("新增子分类") : QStringLiteral("新增根分类"),
                           parentLabel, {}, &body)) {
        return;
    }
    body["parentId"] = parentId;
    ApiClient::instance().post("/knowledge/category", body, [this](const ApiResponse &r) {
        r.ok ? refreshCategories() : setCatStatus(r.message, true);
    });
}

void KnowledgeBasePage::editCategory() {
    qint64 id = selectedCategoryId();
    if (id <= 0) {
        setCatStatus(QStringLiteral("请先选择一个分类"), true);
        return;
    }
    const QString path = "/knowledge/category/" + QString::number(id);
    ApiClient::instance().get(path, [this, path](const ApiResponse &r) {
        if (!r.ok) {
            setCatStatus(r.message, true);
            return;
        }
        const QJsonObject initial = r.object();
        const qint64 parentId = static_cast<qint64>(initial.value("parentId").toDouble());
        const QString parentLabel = parentId <= 0 ? QStringLiteral("（根分类）") : categoryNameById(parentId);
        QJsonObject body;
        if (!runCategoryDialog(this, QStringLiteral("编辑分类"), parentLabel, initial, &body)) {
            return;
        }
        body["parentId"] = parentId; // 保持原层级，迁移不在本页范围
        ApiClient::instance().put(path, body, [this](const ApiResponse &wr) {
            wr.ok ? refreshCategories() : setCatStatus(wr.message, true);
        });
    });
}

void KnowledgeBasePage::deleteCategory() {
    qint64 id = selectedCategoryId();
    if (id <= 0) {
        setCatStatus(QStringLiteral("请先选择一个分类"), true);
        return;
    }
    QTreeWidgetItem *item = m_catTree->currentItem();
    const bool hasChildren = item && item->childCount() > 0;
    if (hasChildren) {
        // 含下级分类：级联删除有破坏性，要求输入「确定删除」二次确认。
        bool ok = false;
        const QString input = QInputDialog::getText(
            this, QStringLiteral("确认删除"),
            QStringLiteral("分类「%1」含下级分类，删除将一并移除其全部子分类。\n请输入「确定删除」以确认：")
                .arg(selectedCategoryName()),
            QLineEdit::Normal, QString(), &ok);
        if (!ok) {
            return;
        }
        if (input.trimmed() != QStringLiteral("确定删除")) {
            setCatStatus(QStringLiteral("输入不匹配，已取消删除"), true);
            return;
        }
    } else if (QMessageBox::question(this, QStringLiteral("确认删除"),
                                     QStringLiteral("确认删除分类「%1」？").arg(selectedCategoryName()))
               != QMessageBox::Yes) {
        return;
    }
    ApiClient::instance().del("/knowledge/category/" + QString::number(id), [this](const ApiResponse &r) {
        r.ok ? refreshCategories() : setCatStatus(r.message, true);
    });
}

void KnowledgeBasePage::refreshTags() {
    setTagStatus(QStringLiteral("加载中..."));
    ApiClient::instance().get("/knowledge/tag?page=1&pageSize=100", [this](const ApiResponse &r) {
        if (!r.ok) {
            setTagStatus(r.message, true);
            return;
        }
        const QJsonArray list = r.object().value("list").toArray();
        m_tagTable->setColumnCount(3);
        m_tagTable->setHorizontalHeaderLabels({QStringLiteral("名称"), QStringLiteral("颜色"),
                                               QStringLiteral("排序")});
        TableStyle::configureTagTable(m_tagTable);
        m_tagTable->setRowCount(0);
        for (const QJsonValue &v : list) {
            const QJsonObject o = v.toObject();
            const int row = m_tagTable->rowCount();
            m_tagTable->insertRow(row);

            auto *nameItem = new QTableWidgetItem(o.value("name").toString());
            nameItem->setData(Qt::UserRole, o.value("id").toVariant());
            m_tagTable->setItem(row, 0, nameItem);

            const QString hex = o.value("color").toString();
            m_tagTable->setCellWidget(row, 1, makeTagColorCell(hex, m_tagTable));

            m_tagTable->setItem(row, 2, new QTableWidgetItem(textOf(o, "sort")));
        }
        TableStyle::setItemTooltipFromText(m_tagTable);
        setTagStatus(QStringLiteral("已加载 %1 个标签").arg(list.size()));
    });
}

void KnowledgeBasePage::addTag() {
    QJsonObject body;
    if (!runTagDialog(this, QStringLiteral("新增标签"), {}, &body)) {
        return;
    }
    ApiClient::instance().post("/knowledge/tag", body, [this](const ApiResponse &r) {
        r.ok ? refreshTags() : setTagStatus(r.message, true);
    });
}

void KnowledgeBasePage::editTag() {
    qint64 id = selectedTagId();
    if (id <= 0) {
        setTagStatus(QStringLiteral("请先选择一个标签"), true);
        return;
    }
    const QString path = "/knowledge/tag/" + QString::number(id);
    ApiClient::instance().get(path, [this, path](const ApiResponse &r) {
        if (!r.ok) {
            setTagStatus(r.message, true);
            return;
        }
        QJsonObject body;
        if (!runTagDialog(this, QStringLiteral("编辑标签"), r.object(), &body)) {
            return;
        }
        ApiClient::instance().put(path, body, [this](const ApiResponse &wr) {
            wr.ok ? refreshTags() : setTagStatus(wr.message, true);
        });
    });
}

void KnowledgeBasePage::deleteTag() {
    qint64 id = selectedTagId();
    if (id <= 0) {
        setTagStatus(QStringLiteral("请先选择一个标签"), true);
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("确认删除"), QStringLiteral("确认删除选中标签？"))
        != QMessageBox::Yes) {
        return;
    }
    ApiClient::instance().del("/knowledge/tag/" + QString::number(id), [this](const ApiResponse &r) {
        r.ok ? refreshTags() : setTagStatus(r.message, true);
    });
}

qint64 KnowledgeBasePage::selectedCategoryId() const {
    QTreeWidgetItem *item = m_catTree ? m_catTree->currentItem() : nullptr;
    return item ? item->data(0, Qt::UserRole).toLongLong() : 0;
}

QString KnowledgeBasePage::selectedCategoryName() const {
    QTreeWidgetItem *item = m_catTree ? m_catTree->currentItem() : nullptr;
    return item ? item->text(0) : QString();
}

QString KnowledgeBasePage::categoryNameById(qint64 id) const {
    if (!m_catTree) {
        return QStringLiteral("#%1").arg(id);
    }
    QTreeWidgetItemIterator it(m_catTree);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toLongLong() == id) {
            return (*it)->text(0);
        }
        ++it;
    }
    return QStringLiteral("#%1").arg(id);
}

qint64 KnowledgeBasePage::selectedTagId() const {
    if (!m_tagTable || m_tagTable->currentRow() < 0 || !m_tagTable->item(m_tagTable->currentRow(), 0)) {
        return 0;
    }
    return m_tagTable->item(m_tagTable->currentRow(), 0)->data(Qt::UserRole).toLongLong();
}

void KnowledgeBasePage::setCatStatus(const QString &text, bool error) {
    if (!m_catStatus) {
        return;
    }
    m_catStatus->setText(text);
    m_catStatus->setStyleSheet(error ? "color:#B94A48;" : "color:#757575;");
    if (error && !text.isEmpty()) {
        notify::warn(this, text);
    }
}

void KnowledgeBasePage::setTagStatus(const QString &text, bool error) {
    if (!m_tagStatus) {
        return;
    }
    m_tagStatus->setText(text);
    m_tagStatus->setStyleSheet(error ? "color:#B94A48;" : "color:#757575;");
    if (error && !text.isEmpty()) {
        notify::warn(this, text);
    }
}

} // namespace kb
