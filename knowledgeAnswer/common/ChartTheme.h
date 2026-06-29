#pragma once

#include <QColor>
#include <QString>
#include <QStringList>
#include <QVector>

class QChart;

namespace kb {
namespace ChartTheme {

QVector<QColor> palette();
void applyBase(QChart *chart, bool showTitle = false);
QChart *emptyChart(const QString &hint = QString());

/** 折线图：categories 与 series 等长；点较多时自动稀疏 X 轴标签。 */
QChart *buildLineChart(const QStringList &categories,
                       const QVector<QPair<QString, QVector<qint64>>> &series);

/** 饼图（无内置标题/图例，图例由 ChartCard 顶部展示）。 */
QChart *buildPieChart(const QVector<QPair<QString, qint64>> &slices);

QString compactSeriesLegendHtml(const QStringList &names);
QString compactLegendHtml(const QVector<QPair<QString, qint64>> &slices);

QChart *buildHorizontalBarChart(const QStringList &labels,
                                const QVector<qint64> &values,
                                const QColor &barColor = QColor(),
                                int maxLeftMargin = 120,
                                const QStringList &tooltipLabels = QStringList());

QString statusLabel(const QString &code);
QString typeLabel(const QString &code);

} // namespace ChartTheme
} // namespace kb
