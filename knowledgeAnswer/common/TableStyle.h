#pragma once

class QHeaderView;
class QTableWidget;
class QTreeWidget;

namespace kb {

/** 全站 QTableWidget / QTreeWidget（DataTable）统一样式与列宽策略。 */
namespace TableStyle {

void applyDataTable(QTableWidget *table);
void applyDataTree(QTreeWidget *tree);

/** 分类树：名称/编码自适应，排序/状态按内容。 */
void configureCategoryTree(QTreeWidget *tree);

/** 标签表：名称拉伸，颜色固定宽，排序按内容。 */
void configureTagTable(QTableWidget *table);

/** 标题列拉伸，其余列按内容（适用于收藏/管理等列表）。 */
void configureTitleTable(QTableWidget *table, int titleColumn = 0);

/** 带序号列的榜单：# 固定宽，标题拉伸，其余按内容。 */
void configureRankedTable(QTableWidget *table, int titleColumn = 1);

/**
 * 多列宽表：列按内容展开，超出视口时表格内横向滚动（不撑大窗体）。
 * @param fixedColumn 固定宽列（如 # 列传 0；无则传 -1）
 */
void configureWideScrollTable(QTableWidget *table, int fixedColumn = -1);

/** 配合 configureWideScrollTable，在填充数据后调用。 */
void expandWideScrollColumns(QTableWidget *table, int fixedColumn = -1);

void setItemTooltipFromText(QTableWidget *table);

/** 分类树：名称/编码列悬停显示完整文本。 */
void setTreeItemTooltipFromText(QTreeWidget *tree);

} // namespace TableStyle

} // namespace kb
