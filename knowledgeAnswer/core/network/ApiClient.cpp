#include "core/network/ApiClient.h"

#include "core/auth/Session.h"
#include "core/config/Settings.h"
#include "core/logging/Logger.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace kb {

ApiClient &ApiClient::instance() {
    static ApiClient *inst = new ApiClient();
    return *inst;
}

ApiClient::ApiClient(QObject *parent) : QObject(parent) {}

QNetworkRequest ApiClient::buildRequest(const QString &path) const {
    QUrl url(Settings::instance().baseUrl() + path);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    const QString tok = Session::instance().token();
    if (!tok.isEmpty()) {
        req.setRawHeader("Authorization", QByteArray("Bearer ") + tok.toUtf8());
    }
    return req;
}

void ApiClient::handle(QNetworkReply *reply, Callback cb) {
    QObject::connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        ApiResponse res;
        const QByteArray raw = reply->readAll();
        const auto netErr = reply->error();

        QJsonParseError pe{};
        const QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
        if (pe.error == QJsonParseError::NoError && doc.isObject()) {
            const QJsonObject o = doc.object();
            res.code = o.value("code").toInt(-1);
            res.message = o.value("message").toString();
            res.data = o.value("data");
            res.ok = (res.code == 0);
        } else if (netErr != QNetworkReply::NoError) {
            res.networkError = true;
            res.message = reply->errorString();
        } else {
            res.message = QStringLiteral("响应解析失败");
        }

        if (!res.ok) {
            kb::log::warn(QStringLiteral("API %1 -> code=%2 msg=%3")
                              .arg(reply->url().toString())
                              .arg(res.code)
                              .arg(res.message));
        }

        reply->deleteLater();
        if (cb) {
            cb(res);
        }
    });
}

void ApiClient::get(const QString &path, Callback cb) {
    handle(m_manager.get(buildRequest(path)), std::move(cb));
}

void ApiClient::post(const QString &path, const QJsonObject &body, Callback cb) {
    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
    handle(m_manager.post(buildRequest(path), payload), std::move(cb));
}

void ApiClient::put(const QString &path, const QJsonObject &body, Callback cb) {
    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
    handle(m_manager.put(buildRequest(path), payload), std::move(cb));
}

void ApiClient::del(const QString &path, Callback cb) {
    handle(m_manager.deleteResource(buildRequest(path)), std::move(cb));
}

} // namespace kb
