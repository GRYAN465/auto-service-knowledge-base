#include "common/ChartCard.h"

#include <QChartView>
#include <QLabel>
#include <QPainter>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidget>

namespace kb {

ChartCard::ChartCard(const QString &title, int minChartHeight, QWidget *parent)
    : QFrame(parent) {
    setObjectName("ChartSection");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_rootLayout = new QVBoxLayout(this);
    m_rootLayout->setContentsMargins(20, 18, 20, 18);
    m_rootLayout->setSpacing(6);

    m_title = new QLabel(title, this);
    m_title->setObjectName("SectionTitle");
    m_rootLayout->addWidget(m_title);

    m_subtitle = new QLabel(this);
    m_subtitle->setObjectName("MutedLine");
    m_subtitle->setWordWrap(true);
    m_subtitle->hide();
    m_rootLayout->addWidget(m_subtitle);

    m_bodyLayout = new QHBoxLayout();
    m_bodyLayout->setSpacing(10);
    m_bodyLayout->setContentsMargins(0, 0, 0, 0);

    m_legend = new QLabel(this);
    m_legend->setObjectName("ChartLegend");
    m_legend->setWordWrap(true);
    m_legend->setTextFormat(Qt::RichText);
    m_legend->hide();

    m_view = new QChartView(this);
    m_view->setObjectName("ChartView");
    m_view->setMinimumHeight(minChartHeight);
    m_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_view->setMouseTracking(true);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_bodyLayout->addWidget(m_view, 1);
    m_rootLayout->addLayout(m_bodyLayout, 1);
}

void ChartCard::rebuildLegendPlacement() {
    if (!m_bodyLayout || !m_legend || !m_rootLayout) {
        return;
    }
    m_bodyLayout->removeWidget(m_legend);
    if (m_rootLayout->indexOf(m_legend) >= 0) {
        m_rootLayout->removeWidget(m_legend);
    }

    if (!m_legend->isVisible() || m_legendStyle == ChartLegendStyle::Hidden) {
        return;
    }

    if (m_legendStyle == ChartLegendStyle::LeftVertical) {
        m_legend->setProperty("vertical", true);
        m_legend->setProperty("bottom", false);
        m_legend->setMaximumWidth(140);
        m_legend->setMinimumWidth(96);
        m_legend->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        m_bodyLayout->insertWidget(0, m_legend, 0, Qt::AlignTop);
    } else {
        m_legend->setProperty("vertical", false);
        m_legend->setProperty("bottom", true);
        m_legend->setMaximumWidth(QWIDGETSIZE_MAX);
        m_legend->setMinimumWidth(0);
        m_legend->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
        m_rootLayout->addWidget(m_legend);
    }
    m_legend->style()->unpolish(m_legend);
    m_legend->style()->polish(m_legend);
}

void ChartCard::setLegendStyle(ChartLegendStyle style) {
    if (m_legendStyle == style) {
        return;
    }
    m_legendStyle = style;
    rebuildLegendPlacement();
}

void ChartCard::setLegendVertical(bool vertical) {
    setLegendStyle(vertical ? ChartLegendStyle::LeftVertical : ChartLegendStyle::Bottom);
}

void ChartCard::setChartMinHeight(int height) {
    if (m_view) {
        m_view->setMinimumHeight(height);
    }
}

void ChartCard::setChart(QChart *chart) {
    if (!m_view) {
        return;
    }
    if (QChart *old = m_view->chart()) {
        old->deleteLater();
    }
    m_view->setChart(chart);
}

void ChartCard::setLegendHtml(const QString &html) {
    if (!m_legend) {
        return;
    }
    const bool show = !html.trimmed().isEmpty();
    const bool wasVisible = m_legend->isVisible();
    m_legend->setVisible(show);
    if (show) {
        m_legend->setText(html);
        if (m_legendStyle == ChartLegendStyle::Hidden) {
            m_legendStyle = ChartLegendStyle::Bottom;
        }
    }
    if (show != wasVisible || show) {
        rebuildLegendPlacement();
    }
}

void ChartCard::setSubtitle(const QString &text) {
    if (!m_subtitle) {
        return;
    }
    const bool show = !text.trimmed().isEmpty();
    m_subtitle->setVisible(show);
    if (show) {
        m_subtitle->setText(text);
    }
}

} // namespace kb
