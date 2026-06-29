#include "app/ArticleEditorDialog.h"

#include "common/TableStyle.h"
#include "common/ComboStyle.h"
#include "common/ThemeIcons.h"
#include "core/auth/Session.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSet>
#include <QSplitter>
#include <QTableWidget>
#include <QTextCharFormat>
#include <QTextEdit>
#include <QTextListFormat>
#include <QVBoxLayout>
#include <functional>

namespace kb {

namespace {

struct TypeOption {
    const char *code;
    const char *label;
};
const TypeOption kTypeOptions[] = {
    {"SCRIPT", "话术"}, {"TRAIN", "培训"}, {"PRODUCT", "产品"}, {"OFFICE", "办公"}};

QString humanSize(qint64 bytes) {
    if (bytes <= 0) {
        return QStringLiteral("-");
    }
    if (bytes < 1024) {
        return QString::number(bytes) + " B";
    }
    if (bytes < 1024 * 1024) {
        return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    }
    return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
}

void flattenCategories(QComboBox *box, const QJsonArray &nodes, int depth) {
    for (const QJsonValue &v : nodes) {
        const QJsonObject o = v.toObject();
        const QString indent = QString(depth * 2, QChar(' '));
        box->addItem(indent + o.value("name").toString(),
                     QVariant::fromValue<qint64>(static_cast<qint64>(o.value("id").toDouble())));
        flattenCategories(box, o.value("children").toArray(), depth + 1);
    }
}

} // namespace

ArticleEditorDialog::ArticleEditorDialog(qint64 articleId, QWidget *parent)
    : QDialog(parent), m_articleId(articleId) {
    setWindowTitle(articleId > 0 ? QStringLiteral("编辑知识") : QStringLiteral("新建知识"));
    setMinimumSize(820, 640);
    buildUi();
    loadCategories();
    loadTags();
    if (m_articleId > 0) {
        loadDetailIfEditing();
        refreshAttachments();
        setAttachmentPanelEnabled(true);
    } else {
        setAttachmentPanelEnabled(false);
    }
}

void ArticleEditorDialog::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 18, 20, 18);
    root->setSpacing(12);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    m_title = new QLineEdit(this);
    form->addRow(QStringLiteral("标题"), m_title);

    m_category = new QComboBox(this);
    m_category->addItem(QStringLiteral("（无分类）"), QVariant::fromValue<qint64>(0));
    form->addRow(QStringLiteral("分类"), m_category);

    m_type = new QComboBox(this);
    for (const auto &opt : kTypeOptions) {
        m_type->addItem(QString::fromUtf8(opt.label), QString::fromLatin1(opt.code));
    }
    form->addRow(QStringLiteral("类型"), m_type);
    ComboStyle::polish(m_category);
    ComboStyle::polish(m_type);

    m_summary = new QLineEdit(this);
    m_summary->setPlaceholderText(QStringLiteral("一句话摘要（可选）"));
    form->addRow(QStringLiteral("摘要"), m_summary);
    root->addLayout(form);

    auto *splitter = new QSplitter(Qt::Horizontal, this);

    auto *tagBox = new QWidget(splitter);
    auto *tagLayout = new QVBoxLayout(tagBox);
    tagLayout->setContentsMargins(0, 0, 0, 0);
    tagLayout->addWidget(new QLabel(QStringLiteral("标签"), tagBox));
    m_tagList = new QListWidget(tagBox);
    m_tagList->setObjectName("DataTable");
    tagLayout->addWidget(m_tagList, 1);
    splitter->addWidget(tagBox);

    auto *contentBox = new QWidget(splitter);
    auto *contentLayout = new QVBoxLayout(contentBox);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(6);

    auto *toolbar = new QHBoxLayout();
    auto *bold = new QPushButton(QStringLiteral("B"), contentBox);
    auto *italic = new QPushButton(QStringLiteral("I"), contentBox);
    auto *underline = new QPushButton(QStringLiteral("U"), contentBox);
    auto *h2 = new QPushButton(QStringLiteral("H2"), contentBox);
    auto *bullet = new QPushButton(QStringLiteral("• 列表"), contentBox);
    for (QPushButton *b : {bold, italic, underline, h2, bullet}) {
        b->setObjectName("GhostButton");
        b->setFixedHeight(28);
        toolbar->addWidget(b);
    }
    toolbar->addStretch();
    contentLayout->addLayout(toolbar);

    m_content = new QTextEdit(contentBox);
    m_content->setObjectName("DataTable");
    m_content->setAcceptRichText(true);
    contentLayout->addWidget(m_content, 1);
    splitter->addWidget(contentBox);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    root->addWidget(splitter, 1);

    connect(bold, &QPushButton::clicked, this, [this]() {
        QTextCharFormat f;
        f.setFontWeight(m_content->fontWeight() > QFont::Normal ? QFont::Normal : QFont::Bold);
        m_content->mergeCurrentCharFormat(f);
    });
    connect(italic, &QPushButton::clicked, this, [this]() {
        QTextCharFormat f;
        f.setFontItalic(!m_content->fontItalic());
        m_content->mergeCurrentCharFormat(f);
    });
    connect(underline, &QPushButton::clicked, this, [this]() {
        QTextCharFormat f;
        f.setFontUnderline(!m_content->fontUnderline());
        m_content->mergeCurrentCharFormat(f);
    });
    connect(h2, &QPushButton::clicked, this, [this]() {
        QTextCharFormat f;
        f.setFontPointSize(16);
        f.setFontWeight(QFont::Bold);
        m_content->mergeCurrentCharFormat(f);
    });
    connect(bullet, &QPushButton::clicked, this, [this]() {
        m_content->textCursor().createList(QTextListFormat::ListDisc);
    });

    m_attachHint = new QLabel(QStringLiteral("附件"), this);
    root->addWidget(m_attachHint);
    m_attachTable = new QTableWidget(this);
    m_attachTable->setColumnCount(4);
    m_attachTable->setHorizontalHeaderLabels(
        {QStringLiteral("文件名"), QStringLiteral("类型"), QStringLiteral("大小"), QStringLiteral("下载")});
    TableStyle::configureTitleTable(m_attachTable, 0);
    m_attachTable->setMaximumHeight(140);
    root->addWidget(m_attachTable);

    auto *attachBar = new QHBoxLayout();
    m_attachUpload = new QPushButton(this);
    m_attachDownload = new QPushButton(this);
    m_attachDelete = new QPushButton(this);
    ThemeIcons::applyIconButton(m_attachUpload, ThemeIcons::Kind::Upload, QStringLiteral("上传附件"));
    ThemeIcons::applyIconButton(m_attachDownload, ThemeIcons::Kind::Download, QStringLiteral("下载附件"));
    ThemeIcons::applyIconButton(m_attachDelete, ThemeIcons::Kind::Trash, QStringLiteral("删除附件"));
    attachBar->addWidget(m_attachUpload);
    attachBar->addWidget(m_attachDownload);
    attachBar->addWidget(m_attachDelete);
    attachBar->addStretch();
    root->addLayout(attachBar);
    connect(m_attachUpload, &QPushButton::clicked, this, &ArticleEditorDialog::uploadAttachment);
    connect(m_attachDownload, &QPushButton::clicked, this, &ArticleEditorDialog::downloadAttachment);
    connect(m_attachDelete, &QPushButton::clicked, this, &ArticleEditorDialog::deleteAttachment);

    m_status = new QLabel(this);
    m_status->setObjectName("StatusLabel");
    root->addWidget(m_status);

    auto *bottom = new QHBoxLayout();
    m_saveButton = new QPushButton(QStringLiteral("保存草稿"), this);
    m_saveButton->setObjectName("PrimaryButton");
    m_submitButton = new QPushButton(QStringLiteral("提交审核"), this);
    m_submitButton->setObjectName("GhostButton");
    m_submitButton->setVisible(Session::instance().hasPermission(QStringLiteral("knowledge:article:submit")));
    auto *closeButton = new QPushButton(QStringLiteral("关闭"), this);
    closeButton->setObjectName("GhostButton");
    bottom->addStretch();
    bottom->addWidget(m_saveButton);
    bottom->addWidget(m_submitButton);
    bottom->addWidget(closeButton);
    root->addLayout(bottom);
    connect(m_saveButton, &QPushButton::clicked, this, &ArticleEditorDialog::saveDraft);
    connect(m_submitButton, &QPushButton::clicked, this, &ArticleEditorDialog::submitAudit);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

QString ArticleEditorDialog::detailApiPath() const {
    if (m_articleId <= 0) {
        return {};
    }
    if (Session::instance().hasPermission(QStringLiteral("knowledge:article:list"))) {
        return "/knowledge/article/" + QString::number(m_articleId);
    }
    return "/knowledge/article/mine/" + QString::number(m_articleId);
}

void ArticleEditorDialog::loadCategories() {
    ApiClient::instance().get("/knowledge/category/tree", [this](const ApiResponse &r) {
        if (r.ok) {
            flattenCategories(m_category, r.data.toArray(), 0);
            if (m_articleId > 0) {
                loadDetailIfEditing();
            }
        }
    });
}

void ArticleEditorDialog::loadTags() {
    ApiClient::instance().get("/knowledge/tag?page=1&pageSize=100", [this](const ApiResponse &r) {
        if (!r.ok) {
            return;
        }
        m_tagList->clear();
        const QJsonArray list = r.object().value("list").toArray();
        for (const QJsonValue &v : list) {
            const QJsonObject o = v.toObject();
            auto *item = new QListWidgetItem(o.value("name").toString(), m_tagList);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
            item->setData(Qt::UserRole, o.value("id").toVariant());
        }
        if (m_articleId > 0) {
            loadDetailIfEditing();
        }
    });
}

void ArticleEditorDialog::loadDetailIfEditing() {
    if (m_articleId <= 0) {
        return;
    }
    ApiClient::instance().get(detailApiPath(), [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        const QJsonObject d = r.object();
        m_title->setText(d.value("title").toString());
        m_summary->setText(d.value("summary").toString());
        m_content->setHtml(d.value("content").toString());

        const qint64 catId = static_cast<qint64>(d.value("categoryId").toDouble());
        for (int i = 0; i < m_category->count(); ++i) {
            if (m_category->itemData(i).toLongLong() == catId) {
                m_category->setCurrentIndex(i);
                break;
            }
        }
        const QString type = d.value("knowledgeType").toString();
        int typeIdx = m_type->findData(type);
        if (typeIdx >= 0) {
            m_type->setCurrentIndex(typeIdx);
        }
        QSet<qint64> tagIds;
        for (const QJsonValue &tv : d.value("tags").toArray()) {
            tagIds.insert(static_cast<qint64>(tv.toObject().value("id").toDouble()));
        }
        for (int i = 0; i < m_tagList->count(); ++i) {
            QListWidgetItem *item = m_tagList->item(i);
            item->setCheckState(tagIds.contains(item->data(Qt::UserRole).toLongLong())
                                    ? Qt::Checked : Qt::Unchecked);
        }
    });
}

QJsonObject ArticleEditorDialog::collectBody() const {
    QJsonObject body;
    body["title"] = m_title->text().trimmed();
    const qint64 catId = m_category->currentData().toLongLong();
    body["categoryId"] = catId > 0 ? QJsonValue(catId) : QJsonValue();
    body["knowledgeType"] = m_type->currentData().toString();
    body["summary"] = m_summary->text().trimmed();
    body["content"] = m_content->toHtml();
    QJsonArray tagIds;
    for (int i = 0; i < m_tagList->count(); ++i) {
        QListWidgetItem *item = m_tagList->item(i);
        if (item->checkState() == Qt::Checked) {
            tagIds.append(item->data(Qt::UserRole).toLongLong());
        }
    }
    body["tagIds"] = tagIds;
    return body;
}

void ArticleEditorDialog::submitForAudit(qint64 articleId) {
    ApiClient::instance().post("/knowledge/article/" + QString::number(articleId) + "/submit",
                               QJsonObject{}, [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        m_dirty = true;
        notify::info(this, QStringLiteral("已提交审核，审核通过后将展示在社区"));
        accept();
    });
}

void ArticleEditorDialog::persistDraft(const std::function<void(bool ok)> &then) {
    if (m_title->text().trimmed().isEmpty()) {
        setStatus(QStringLiteral("标题不能为空"), true);
        then(false);
        return;
    }
    const QJsonObject body = collectBody();
    if (m_articleId > 0) {
        ApiClient::instance().put("/knowledge/article/" + QString::number(m_articleId), body,
                                  [this, then](const ApiResponse &r) {
            if (!r.ok) {
                setStatus(r.message, true);
                then(false);
                return;
            }
            m_dirty = true;
            setStatus(QStringLiteral("已保存（版本 %1）").arg(r.object().value("currentVersion").toInt()));
            then(true);
        });
    } else {
        ApiClient::instance().post("/knowledge/article", body, [this, then](const ApiResponse &r) {
            if (!r.ok) {
                setStatus(r.message, true);
                then(false);
                return;
            }
            m_articleId = static_cast<qint64>(r.object().value("id").toDouble());
            m_dirty = true;
            setWindowTitle(QStringLiteral("编辑知识"));
            setAttachmentPanelEnabled(true);
            refreshAttachments();
            setStatus(QStringLiteral("草稿已保存，现在可以上传附件"));
            then(true);
        });
    }
}

void ArticleEditorDialog::saveDraft() {
    persistDraft([](bool) {});
}

void ArticleEditorDialog::submitAudit() {
    persistDraft([this](bool ok) {
        if (ok && m_articleId > 0) {
            submitForAudit(m_articleId);
        }
    });
}

void ArticleEditorDialog::refreshAttachments() {
    if (m_articleId <= 0) {
        return;
    }
    ApiClient::instance().get("/knowledge/attachment?articleId=" + QString::number(m_articleId),
                              [this](const ApiResponse &r) {
        if (!r.ok) {
            return;
        }
        m_attachTable->setRowCount(0);
        for (const QJsonValue &v : r.data.toArray()) {
            const QJsonObject o = v.toObject();
            const int row = m_attachTable->rowCount();
            m_attachTable->insertRow(row);
            auto *nameItem = new QTableWidgetItem(o.value("fileName").toString());
            nameItem->setData(Qt::UserRole, o.value("id").toVariant());
            m_attachTable->setItem(row, 0, nameItem);
            m_attachTable->setItem(row, 1, new QTableWidgetItem(o.value("fileType").toString()));
            m_attachTable->setItem(row, 2, new QTableWidgetItem(
                humanSize(static_cast<qint64>(o.value("fileSize").toDouble()))));
            m_attachTable->setItem(row, 3, new QTableWidgetItem(
                QString::number(static_cast<qint64>(o.value("downloadCount").toDouble()))));
        }
        TableStyle::setItemTooltipFromText(m_attachTable);
    });
}

void ArticleEditorDialog::uploadAttachment() {
    if (m_articleId <= 0) {
        setStatus(QStringLiteral("请先保存草稿再上传附件"), true);
        return;
    }
    const QString file = QFileDialog::getOpenFileName(this, QStringLiteral("选择附件"));
    if (file.isEmpty()) {
        return;
    }
    QJsonObject extra;
    extra["articleId"] = m_articleId;
    setStatus(QStringLiteral("上传中..."));
    ApiClient::instance().upload("/knowledge/attachment/upload?articleId=" + QString::number(m_articleId),
                                 file, QStringLiteral("file"), extra, [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        m_dirty = true;
        refreshAttachments();
        setStatus(QStringLiteral("附件已上传"));
    });
}

void ArticleEditorDialog::downloadAttachment() {
    const qint64 id = selectedAttachmentId();
    if (id <= 0) {
        setStatus(QStringLiteral("请先选择附件"), true);
        return;
    }
    const QString suggested = m_attachTable->item(m_attachTable->currentRow(), 0)->text();
    const QString savePath = QFileDialog::getSaveFileName(this, QStringLiteral("保存附件"), suggested);
    if (savePath.isEmpty()) {
        return;
    }
    setStatus(QStringLiteral("下载中..."));
    ApiClient::instance().download("/knowledge/attachment/" + QString::number(id) + "/download",
                                   savePath, [this](bool ok, const QString &err) {
        if (!ok) {
            setStatus(QStringLiteral("下载失败：%1").arg(err), true);
            return;
        }
        refreshAttachments();
        setStatus(QStringLiteral("已下载"));
    });
}

void ArticleEditorDialog::deleteAttachment() {
    const qint64 id = selectedAttachmentId();
    if (id <= 0) {
        setStatus(QStringLiteral("请先选择附件"), true);
        return;
    }
    ApiClient::instance().del("/knowledge/attachment/" + QString::number(id), [this](const ApiResponse &r) {
        if (!r.ok) {
            setStatus(r.message, true);
            return;
        }
        m_dirty = true;
        refreshAttachments();
        setStatus(QStringLiteral("附件已删除"));
    });
}

void ArticleEditorDialog::setAttachmentPanelEnabled(bool enabled) {
    m_attachUpload->setEnabled(enabled);
    m_attachDownload->setEnabled(enabled);
    m_attachDelete->setEnabled(enabled);
    m_attachHint->setText(enabled ? QStringLiteral("附件")
                                   : QStringLiteral("附件（保存草稿后可上传）"));
}

qint64 ArticleEditorDialog::selectedAttachmentId() const {
    const int row = m_attachTable->currentRow();
    if (row < 0 || !m_attachTable->item(row, 0)) {
        return 0;
    }
    return m_attachTable->item(row, 0)->data(Qt::UserRole).toLongLong();
}

void ArticleEditorDialog::setStatus(const QString &text, bool error) {
    m_status->setText(text);
    m_status->setStyleSheet(error ? "color:#B94A48;" : "color:#757575;");
}

} // namespace kb
