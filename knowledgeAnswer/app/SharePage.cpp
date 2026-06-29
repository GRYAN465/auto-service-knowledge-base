#include "app/SharePage.h"

#include "app/ArticleDetailDialog.h"
#include "common/TableStyle.h"
#include "core/network/ApiClient.h"
#include "core/notify/Notify.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

namespace kb {

namespace {

QTableWidget *makeTable(const QStringList &headers) {
    auto *t = new QTableWidget();
    t->setColumnCount(headers.size());
    t->setHorizontalHeaderLabels(headers);
    TableStyle::configureTitleTable(t, 1);
    return t;
}

} // namespace

SharePage::SharePage(const QString &title, QWidget *parent)
    : QWidget(parent), m_title(title) {
    buildUi();
    refresh();
}

void SharePage::refreshPage() {
    refresh();
}

void SharePage::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 24);
    root->setSpacing(12);

    m_status = new QLabel(this);
    m_status->setObjectName("StatusLabel");
    root->addWidget(m_status);

    auto *tabs = new QTabWidget(this);

    m_inbox = makeTable({QStringLiteral("来自"), QStringLiteral("知识"), QStringLiteral("留言"),
                         QStringLiteral("时间"), QStringLiteral("状态")});
    connect(m_inbox, &QTableWidget::doubleClicked, this, &SharePage::openInboxDetail);
    tabs->addTab(m_inbox, QStringLiteral("收到的分享"));

    m_sent = makeTable({QStringLiteral("发给"), QStringLiteral("知识"), QStringLiteral("留言"),
                        QStringLiteral("时间"), QStringLiteral("状态")});
    tabs->addTab(m_sent, QStringLiteral("我发出的"));

    root->addWidget(tabs, 1);
}

void SharePage::refresh() {
    loadUnread();
    loadInbox();
    loadSent();
}

void SharePage::loadUnread() {
    ApiClient::instance().get("/interaction/share/unread-count", [this](const ApiResponse &r) {
        if (!r.ok) {
            m_status->setText(r.message);
            m_status->setStyleSheet("color:#B94A48;");
            return;
        }
        const qint64 unread = static_cast<qint64>(r.data.toDouble());
        m_status->setText(unread > 0 ? QStringLiteral("未读分享：%1 条").arg(unread)
                                     : QStringLiteral("没有未读分享"));
        m_status->setStyleSheet(unread > 0 ? "color:#6B7F74; font-weight:600;" : "color:#757575;");
    });
}

void SharePage::loadInbox() {
    ApiClient::instance().get("/interaction/share/inbox?page=1&pageSize=50", [this](const ApiResponse &r) {
        if (!r.ok) {
            return;
        }
        const QJsonArray list = r.object().value("list").toArray();
        m_inbox->setRowCount(0);
        for (const QJsonValue &v : list) {
            const QJsonObject o = v.toObject();
            const int row = m_inbox->rowCount();
            m_inbox->insertRow(row);
            QString fromName = o.value("fromUserName").toString();
            if (fromName.isEmpty()) {
                const qint64 fromId = static_cast<qint64>(o.value("fromUserId").toDouble());
                fromName = fromId > 0 ? QStringLiteral("用户#%1").arg(fromId) : QStringLiteral("—");
            }
            auto *fromItem = new QTableWidgetItem(fromName);
            fromItem->setData(Qt::UserRole, o.value("articleId").toVariant());
            fromItem->setData(Qt::UserRole + 1, o.value("id").toVariant());
            m_inbox->setItem(row, 0, fromItem);
            m_inbox->setItem(row, 1, new QTableWidgetItem(o.value("articleTitle").toString()));
            m_inbox->setItem(row, 2, new QTableWidgetItem(o.value("message").toString()));
            m_inbox->setItem(row, 3, new QTableWidgetItem(o.value("createTime").toString().replace('T', ' ')));
            m_inbox->setItem(row, 4, new QTableWidgetItem(o.value("readStatus").toInt() == 1
                                                              ? QStringLiteral("已读")
                                                              : QStringLiteral("未读")));
        }
        TableStyle::setItemTooltipFromText(m_inbox);
    });
}

void SharePage::loadSent() {
    ApiClient::instance().get("/interaction/share/sent?page=1&pageSize=50", [this](const ApiResponse &r) {
        if (!r.ok) {
            m_status->setText(r.message);
            m_status->setStyleSheet(QStringLiteral("color:#B94A48;"));
            return;
        }
        const QJsonArray list = r.object().value("list").toArray();
        m_sent->setRowCount(0);
        for (const QJsonValue &v : list) {
            const QJsonObject o = v.toObject();
            const int row = m_sent->rowCount();
            m_sent->insertRow(row);
            QString toName = o.value("toUserName").toString();
            if (toName.isEmpty()) {
                const qint64 toId = static_cast<qint64>(o.value("toUserId").toDouble());
                toName = toId > 0 ? QStringLiteral("用户#%1").arg(toId) : QStringLiteral("—");
            }
            auto *toItem = new QTableWidgetItem(toName);
            toItem->setData(Qt::UserRole, o.value("articleId").toVariant());
            m_sent->setItem(row, 0, toItem);
            m_sent->setItem(row, 1, new QTableWidgetItem(o.value("articleTitle").toString()));
            m_sent->setItem(row, 2, new QTableWidgetItem(o.value("message").toString()));
            m_sent->setItem(row, 3, new QTableWidgetItem(o.value("createTime").toString().replace('T', ' ')));
            m_sent->setItem(row, 4, new QTableWidgetItem(o.value("readStatus").toInt() == 1
                                                             ? QStringLiteral("对方已读")
                                                             : QStringLiteral("未读")));
        }
        TableStyle::setItemTooltipFromText(m_sent);
    });
}

void SharePage::openInboxDetail() {
    const int row = m_inbox->currentRow();
    if (row < 0 || !m_inbox->item(row, 0)) {
        return;
    }
    const qint64 articleId = m_inbox->item(row, 0)->data(Qt::UserRole).toLongLong();
    const qint64 shareId = m_inbox->item(row, 0)->data(Qt::UserRole + 1).toLongLong();
    if (articleId <= 0) {
        return;
    }
    // 打开即标记该分享为已读，再刷新未读数与列表。
    ApiClient::instance().put("/interaction/share/" + QString::number(shareId) + "/read", {},
                              [this](const ApiResponse &) { refresh(); });
    ArticleDetailDialog dlg(articleId, this);
    dlg.exec();
}

} // namespace kb
