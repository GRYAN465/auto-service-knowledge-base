#include "common/TableStyle.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTreeWidgetItem>

#include <functional>

namespace kb {
namespace TableStyle {

namespace {

constexpr int kDefaultRowHeight = 44;
constexpr int kRankColumnWidth = 52;
constexpr int kTagColorColumnWidth = 200;

void applyHeaderDefaults(QHeaderView *header) {
    header->setHighlightSections(false);
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    header->setMinimumSectionSize(56);
    header->setStretchLastSection(false);
    header->setDefaultSectionSize(kDefaultRowHeight);
}

} // namespace

void applyDataTable(QTableWidget *table) {
    if (!table) {
        return;
    }
    table->setObjectName(QStringLiteral("DataTable"));
    table->setShowGrid(false);
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setWordWrap(false);
    table->setTextElideMode(Qt::ElideRight);
    table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setDefaultSectionSize(kDefaultRowHeight);
    table->verticalHeader()->setMinimumSectionSize(kDefaultRowHeight);

    auto *header = table->horizontalHeader();
    applyHeaderDefaults(header);
    header->setSectionResizeMode(QHeaderView::Interactive);
}

void applyDataTree(QTreeWidget *tree) {
    if (!tree) {
        return;
    }
    tree->setObjectName(QStringLiteral("DataTable"));
    tree->setAlternatingRowColors(true);
    tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    tree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    tree->setUniformRowHeights(false);
    tree->setRootIsDecorated(true);

    tree->header()->setStretchLastSection(false);
    applyHeaderDefaults(tree->header());
}

void configureCategoryTree(QTreeWidget *tree) {
    applyDataTree(tree);
    auto *header = tree->header();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->resizeSection(2, 72);
    header->resizeSection(3, 96);
}

void configureTagTable(QTableWidget *table) {
    applyDataTable(table);
    auto *header = table->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->resizeSection(1, kTagColorColumnWidth);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->resizeSection(2, 72);
}

void configureTitleTable(QTableWidget *table, int titleColumn) {
    applyDataTable(table);
    auto *header = table->horizontalHeader();
    for (int col = 0; col < table->columnCount(); ++col) {
        if (col == titleColumn) {
            header->setSectionResizeMode(col, QHeaderView::Stretch);
        } else {
            header->setSectionResizeMode(col, QHeaderView::ResizeToContents);
        }
    }
}

void configureRankedTable(QTableWidget *table, int titleColumn) {
    applyDataTable(table);
    auto *header = table->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->resizeSection(0, kRankColumnWidth);
    header->setSectionResizeMode(titleColumn, QHeaderView::Stretch);
    for (int col = 0; col < table->columnCount(); ++col) {
        if (col == 0 || col == titleColumn) {
            continue;
        }
        header->setSectionResizeMode(col, QHeaderView::ResizeToContents);
    }
}

void setItemTooltipFromText(QTableWidget *table) {
    if (!table) {
        return;
    }
    for (int row = 0; row < table->rowCount(); ++row) {
        for (int col = 0; col < table->columnCount(); ++col) {
            if (QTableWidgetItem *item = table->item(row, col)) {
                const QString text = item->text();
                if (!text.isEmpty()) {
                    item->setToolTip(text);
                }
                item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            }
        }
    }
}

void setTreeItemTooltipFromText(QTreeWidget *tree) {
    if (!tree) {
        return;
    }
    const QList<int> columns = {0, 1};
    for (int row = 0; row < tree->topLevelItemCount(); ++row) {
        std::function<void(QTreeWidgetItem *)> setOnItem;
        setOnItem = [&](QTreeWidgetItem *item) {
            if (!item) {
                return;
            }
            for (int col : columns) {
                const QString text = item->text(col);
                if (!text.isEmpty()) {
                    item->setToolTip(col, text);
                }
            }
            for (int i = 0; i < item->childCount(); ++i) {
                setOnItem(item->child(i));
            }
        };
        setOnItem(tree->topLevelItem(row));
    }
}

} // namespace TableStyle
} // namespace kb
