#include "common/ChartTheme.h"

#include <QCursor>
#include <QFontMetrics>
#include <QPieSlice>
#include <QToolTip>
#include <QtMath>

#include <QAbstractSeries>
#include <QBarCategoryAxis>
#include <QBarSet>
#include <QChart>
#include <QFont>
#include <QMargins>
#include <QHorizontalBarSeries>
#include <QLegend>
#include <QLineSeries>
#include <QPieSeries>
#include <QValueAxis>

namespace kb {
namespace ChartTheme {

namespace {

int categoryAxisLeftMargin(const QStringList &labels, int cap) {
    const QFont font(QStringLiteral("Microsoft YaHei UI"), 10);
    QFontMetrics fm(font);
    int maxW = 0;
    for (const QString &l : labels) {
        maxW = qMax(maxW, fm.horizontalAdvance(l));
    }
    return qBound(36, maxW + 8, cap);
}

QStringList sparseAxisLabels(const QStringList &labels, int maxVisible) {
    if (labels.size() <= maxVisible) {
        return labels;
    }
    QStringList out;
    out.reserve(labels.size());
    const int n = labels.size();
    const int step = qMax(1, static_cast<int>(qCeil(static_cast<qreal>(n) / maxVisible)));
    for (int i = 0; i < n; ++i) {
        if (i == 0 || i == n - 1 || (i % step) == 0) {
            out << labels.at(i);
        } else {
            out << QString();
        }
    }
    return out;
}

QString compactAxisLabel(const QString &label) {
    if (label.length() == 5 && label.at(2) == QChar('-')) {
        return QStringLiteral("%1日").arg(label.mid(3).toInt());
    }
    return label;
}

QStringList axisDisplayLabels(const QStringList &categories) {
    if (categories.size() <= 7) {
        QStringList out;
        out.reserve(categories.size());
        for (const QString &c : categories) {
            out << compactAxisLabel(c);
        }
        return out;
    }
    const int maxLabels = categories.size() <= 12 ? categories.size() : 8;
    return sparseAxisLabels(categories, maxLabels);
}

void attachLineTooltips(QChart *chart, const QStringList &fullCategories) {
    const bool multi = chart->series().size() > 1;
    for (QAbstractSeries *abs : chart->series()) {
        auto *line = qobject_cast<QLineSeries *>(abs);
        if (!line) {
            continue;
        }
        line->setPointsVisible(true);
        QObject::connect(line, &QLineSeries::hovered, chart,
                         [line, fullCategories, multi](const QPointF &point, bool state) {
                             if (!state) {
                                 QToolTip::hideText();
                                 return;
                             }
                             const int idx = qBound(0, qRound(point.x()), fullCategories.size() - 1);
                             const QString cat = fullCategories.at(idx);
                             const QString val = QString::number(static_cast<qint64>(point.y()));
                             const QString tip = multi
                                                     ? QStringLiteral("%1 | %2 · %3")
                                                           .arg(line->name(), cat, val)
                                                     : QStringLiteral("%1 | %2").arg(cat, val);
                             QToolTip::showText(QCursor::pos(), tip);
                         });
    }
}

void attachBarTooltips(QHorizontalBarSeries *series, const QStringList &tipLabels) {
    QObject::connect(series, &QHorizontalBarSeries::hovered, series,
                     [tipLabels](bool status, int index, QBarSet *set) {
                         if (!status || !set || index < 0 || index >= tipLabels.size()) {
                             QToolTip::hideText();
                             return;
                         }
                         const QString tip =
                             QStringLiteral("%1 | %2").arg(tipLabels.at(index),
                                                           QString::number(static_cast<qint64>(set->at(index))));
                         QToolTip::showText(QCursor::pos(), tip);
                     });
}

void attachPieTooltips(QPieSeries *pie) {
    QObject::connect(pie, &QPieSeries::hovered, pie, [](QPieSlice *slice, bool state) {
        if (!state || !slice) {
            QToolTip::hideText();
            return;
        }
        const QString tip = QStringLiteral("%1 | %2").arg(
            slice->label(), QString::number(static_cast<qint64>(slice->value())));
        QToolTip::showText(QCursor::pos(), tip);
    });
}

} // namespace

QVector<QColor> palette() {
    return {QColor("#9AA69D"), QColor("#6B7F74"), QColor("#8A9690"), QColor("#B8C4BA"),
            QColor("#7A8A82"), QColor("#A8B5AE"), QColor("#C5CDC8"), QColor("#D4DAD6")};
}

void applyBase(QChart *chart, bool showTitle) {
    if (!chart) {
        return;
    }
    if (!showTitle) {
        chart->setTitle(QString());
    }
    chart->setBackgroundVisible(false);
    chart->setMargins(QMargins(8, 4, 8, 8));
    if (showTitle) {
        chart->setTitleFont(QFont(QStringLiteral("Microsoft YaHei UI"), 13, QFont::DemiBold));
    }
    if (QLegend *legend = chart->legend()) {
        legend->setVisible(true);
        legend->setAlignment(Qt::AlignBottom);
        legend->setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 11));
        legend->setLabelColor(QColor("#4A4A4A"));
    }
}

QChart *emptyChart(const QString &hint) {
    auto *chart = new QChart();
    applyBase(chart, true);
    chart->setTitle(hint.isEmpty() ? QStringLiteral("暂无数据") : hint);
    chart->legend()->setVisible(false);
    return chart;
}

QChart *buildLineChart(const QStringList &categories,
                       const QVector<QPair<QString, QVector<qint64>>> &series) {
    if (categories.isEmpty() || series.isEmpty()) {
        return emptyChart();
    }
    auto *chart = new QChart();
    applyBase(chart, false);

    qint64 maxVal = 0;
    for (const auto &s : series) {
        auto *line = new QLineSeries();
        line->setName(s.first);
        for (int i = 0; i < categories.size() && i < s.second.size(); ++i) {
            line->append(i, s.second.at(i));
            maxVal = qMax(maxVal, s.second.at(i));
        }
        chart->addSeries(line);
    }

    const QStringList axisLabels = axisDisplayLabels(categories);
    const bool compactX = categories.size() <= 7;

    auto *axisX = new QBarCategoryAxis();
    axisX->append(axisLabels);
    axisX->setLabelsAngle(compactX ? 0 : (categories.size() > 12 ? 0 : -35));
    axisX->setLabelsFont(QFont(QStringLiteral("Microsoft YaHei UI"), compactX ? 9 : 10));
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->setMargins(QMargins(8, 4, 8, compactX ? 28 : 44));

    auto *axisY = new QValueAxis();
    axisY->setLabelFormat("%d");
    axisY->setMin(0);
    const qint64 yMax = qMax<qint64>(maxVal, 1);
    axisY->setMax(yMax);
    axisY->setLabelsFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10));
    if (yMax <= 5) {
        axisY->setTickCount(static_cast<int>(yMax) + 1);
    }
    chart->addAxis(axisY, Qt::AlignLeft);

    for (QAbstractSeries *s : chart->series()) {
        s->attachAxis(axisX);
        s->attachAxis(axisY);
    }

    chart->legend()->setVisible(false);

    const QVector<QColor> colors = palette();
    int ci = 0;
    for (QAbstractSeries *s : chart->series()) {
        if (auto *line = qobject_cast<QLineSeries *>(s)) {
            QPen pen(colors.at(ci % colors.size()), 2);
            line->setPen(pen);
            ++ci;
        }
    }
    attachLineTooltips(chart, categories);
    return chart;
}

QString compactSeriesLegendHtml(const QStringList &names) {
    if (names.isEmpty()) {
        return {};
    }
    const QVector<QColor> colors = palette();
    QStringList parts;
    for (int i = 0; i < names.size(); ++i) {
        const QString color = colors.at(i % colors.size()).name(QColor::HexRgb);
        parts << QStringLiteral("<span style='color:%1'>&#9632;</span> "
                                "<span style='color:#4A4A4A;font-size:12px'>%2</span>")
                       .arg(color, names.at(i));
    }
    return parts.join(QStringLiteral(" &nbsp; "));
}

QString compactLegendHtml(const QVector<QPair<QString, qint64>> &slices) {
    qint64 total = 0;
    for (const auto &s : slices) {
        total += s.second;
    }
    if (total <= 0) {
        return {};
    }
    const QVector<QColor> colors = palette();
    QStringList parts;
    int i = 0;
    for (const auto &s : slices) {
        if (s.second <= 0) {
            continue;
        }
        const double pct = 100.0 * s.second / total;
        const QString color = colors.at(i % colors.size()).name(QColor::HexRgb);
        parts << QStringLiteral("<span style='color:%1'>&#9632;</span> "
                                "<span style='color:#4A4A4A;font-size:11px'>%2 %3 (%4%)</span>")
                       .arg(color, s.first, QString::number(s.second), QString::number(pct, 'f', 1));
        ++i;
    }
    return parts.join(QStringLiteral("<br/>"));
}

QChart *buildPieChart(const QVector<QPair<QString, qint64>> &slices) {
    qint64 total = 0;
    for (const auto &s : slices) {
        total += s.second;
    }
    if (total <= 0) {
        return emptyChart(QStringLiteral("暂无数据"));
    }
    auto *chart = new QChart();
    applyBase(chart, false);
    chart->setMargins(QMargins(4, 4, 4, 4));

    auto *pie = new QPieSeries();
    pie->setPieSize(0.72);
    pie->setHorizontalPosition(0.5);
    pie->setVerticalPosition(0.5);
    const QVector<QColor> colors = palette();
    int i = 0;
    for (const auto &s : slices) {
        if (s.second <= 0) {
            continue;
        }
        auto *slice = pie->append(s.first, s.second);
        slice->setColor(colors.at(i % colors.size()));
        slice->setLabelVisible(false);
        ++i;
    }
    chart->addSeries(pie);
    chart->legend()->setVisible(false);
    attachPieTooltips(pie);
    return chart;
}

QChart *buildHorizontalBarChart(const QStringList &labels,
                                const QVector<qint64> &values,
                                const QColor &barColor,
                                int maxLeftMargin,
                                const QStringList &tooltipLabels) {
    if (labels.isEmpty() || values.isEmpty()) {
        return emptyChart();
    }
    const QStringList tips = tooltipLabels.isEmpty() ? labels : tooltipLabels;
    auto *chart = new QChart();
    applyBase(chart, false);
    const int leftMargin = categoryAxisLeftMargin(labels, maxLeftMargin);
    chart->setMargins(QMargins(leftMargin, 4, 6, 20));

    const int barCount = labels.size();
    const int rowHeight = 32;

    auto *set = new QBarSet(QStringLiteral("数量"));
    set->setColor(barColor.isValid() ? barColor : QColor("#9AA69D"));
    for (qint64 v : values) {
        *set << v;
    }
    auto *series = new QHorizontalBarSeries();
    series->append(set);
    chart->addSeries(series);
    auto *axisY = new QBarCategoryAxis();
    axisY->append(labels);
    axisY->setLabelsFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    auto *axisX = new QValueAxis();
    axisX->setMin(0);
    qint64 maxVal = 0;
    for (qint64 v : values) {
        maxVal = qMax(maxVal, v);
    }
    axisX->setMax(qMax<qint64>(maxVal, 1));
    axisX->setLabelFormat("%d");
    axisX->setLabelsFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10));
    if (maxVal > 0) {
        axisX->setTickCount(qMin(5, static_cast<int>(maxVal) + 1));
    }
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    chart->legend()->setVisible(false);
    attachBarTooltips(series, tips);
    return chart;
}

QString statusLabel(const QString &code) {
    if (code == "DRAFT") return QStringLiteral("草稿");
    if (code == "PENDING_AUDIT") return QStringLiteral("待审核");
    if (code == "ONLINE") return QStringLiteral("已上线");
    if (code == "OFFLINE") return QStringLiteral("已下线");
    if (code == "REJECTED") return QStringLiteral("已驳回");
    if (code == "PASS") return QStringLiteral("通过");
    if (code == "REJECT") return QStringLiteral("驳回");
    return code;
}

QString typeLabel(const QString &code) {
    if (code == "SCRIPT") return QStringLiteral("话术");
    if (code == "TRAIN") return QStringLiteral("培训");
    if (code == "PRODUCT") return QStringLiteral("产品");
    if (code == "OFFICE") return QStringLiteral("办公");
    return code.isEmpty() ? QStringLiteral("未分类") : code;
}

} // namespace ChartTheme
} // namespace kb
