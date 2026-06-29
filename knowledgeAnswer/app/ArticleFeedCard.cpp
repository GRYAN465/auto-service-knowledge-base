#include "app/ArticleFeedCard.h"

#include "common/TagStyle.h"

#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonValue>
#include <QLabel>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QVBoxLayout>

namespace kb {

namespace {

QString stripHtmlPreview(const QString &raw) {
    QString text = raw;
    text.remove(QRegularExpression(QStringLiteral("<style[^>]*>.*?</style>"),
                                 QRegularExpression::CaseInsensitiveOption
                                     | QRegularExpression::DotMatchesEverythingOption));
    text.remove(QRegularExpression(QStringLiteral("<script[^>]*>.*?</script>"),
                                 QRegularExpression::CaseInsensitiveOption
                                     | QRegularExpression::DotMatchesEverythingOption));
    text.remove(QRegularExpression(QStringLiteral("<[^>]+>")));
    text.replace(QStringLiteral("&nbsp;"), QStringLiteral(" "));
    text.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    text = text.simplified();
    return text;
}

QString previewText(const QJsonObject &o) {
    QString text = o.value("summary").toString().trimmed();
    if (text.isEmpty()) {
        text = o.value("contentPreview").toString().trimmed();
    }
    text = stripHtmlPreview(text);
    if (text.isEmpty()) {
        return {};
    }
    if (text.length() > 20) {
        text = text.left(20) + QStringLiteral("…");
    }
    return text;
}

} // namespace

ArticleFeedCard::ArticleFeedCard(const QJsonObject &article, QWidget *parent)
    : QFrame(parent), m_articleId(static_cast<qint64>(article.value("id").toDouble())) {
    setObjectName("FeedCard");
    setCursor(Qt::PointingHandCursor);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(8);

    auto *title = new QLabel(article.value("title").toString(), this);
    title->setObjectName("FeedCardTitle");
    title->setWordWrap(true);
    layout->addWidget(title);

    const QString author = article.value("authorName").toString();
    if (!author.isEmpty()) {
        auto *authorLabel = new QLabel(QStringLiteral("作者：%1").arg(author), this);
        authorLabel->setObjectName("MutedLabel");
        layout->addWidget(authorLabel);
    }

    const QString previewStr = previewText(article);
    if (!previewStr.isEmpty()) {
        auto *preview = new QLabel(previewStr, this);
        preview->setObjectName("MutedLabel");
        preview->setWordWrap(true);
        layout->addWidget(preview);
    }

    const QJsonArray tags = article.value("tags").toArray();
    if (!tags.isEmpty()) {
        auto *tagRow = new QWidget(this);
        tagRow->setAutoFillBackground(false);
        auto *tagLayout = new QHBoxLayout(tagRow);
        tagLayout->setContentsMargins(0, 0, 0, 0);
        tagLayout->setSpacing(6);
        for (const QJsonValue &v : tags) {
            const QJsonObject t = v.toObject();
            tagLayout->addWidget(TagStyle::createTagNameChip(t.value("name").toString(),
                                                             t.value("color").toString(), tagRow));
        }
        tagLayout->addStretch();
        layout->addWidget(tagRow);
    }

    const qint64 likes = static_cast<qint64>(article.value("likeCount").toDouble());
    const qint64 comments = static_cast<qint64>(article.value("commentCount").toDouble());
    const qint64 views = static_cast<qint64>(article.value("viewCount").toDouble());
    auto *stats = new QLabel(QStringLiteral("👍 %1  ·  💬 %2  ·  👁 %3")
                                 .arg(likes)
                                 .arg(comments)
                                 .arg(views),
                             this);
    stats->setObjectName("FeedCardStats");
    layout->addWidget(stats);
}

void ArticleFeedCard::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_articleId);
    }
    QFrame::mouseReleaseEvent(event);
}

} // namespace kb
