#pragma once

#include <QFrame>

class QChart;
class QChartView;
class QHBoxLayout;
class QLabel;
class QVBoxLayout;

namespace kb {

enum class ChartLegendStyle { Hidden, Bottom, LeftVertical };

/** 统计页图表分区：SectionCard + 标题 + QChartView。 */
class ChartCard : public QFrame {
    Q_OBJECT

public:
    explicit ChartCard(const QString &title, int minChartHeight = 280, QWidget *parent = nullptr);

    void setChart(QChart *chart);
    void setSubtitle(const QString &text);
    void setLegendHtml(const QString &html);
    void setLegendStyle(ChartLegendStyle style);
    /** @deprecated 使用 setLegendStyle(ChartLegendStyle::LeftVertical) */
    void setLegendVertical(bool vertical);
    void setChartMinHeight(int height);

private:
    void rebuildLegendPlacement();

    QLabel *m_title = nullptr;
    QLabel *m_legend = nullptr;
    QLabel *m_subtitle = nullptr;
    QChartView *m_view = nullptr;
    QVBoxLayout *m_rootLayout = nullptr;
    QHBoxLayout *m_bodyLayout = nullptr;
    ChartLegendStyle m_legendStyle = ChartLegendStyle::Hidden;
};

} // namespace kb
