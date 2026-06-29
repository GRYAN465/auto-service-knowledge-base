#pragma once

#include <QFrame>
#include <QJsonObject>

namespace kb {

/** 知识社区信息流卡片。 */
class ArticleFeedCard : public QFrame {
    Q_OBJECT

public:
    explicit ArticleFeedCard(const QJsonObject &article, QWidget *parent = nullptr);

    qint64 articleId() const { return m_articleId; }

signals:
    void clicked(qint64 articleId);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    qint64 m_articleId = 0;
};

} // namespace kb
