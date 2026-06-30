#include "core/network/ApiClient.h"

#include "core/auth/Session.h"
#include "core/config/Settings.h"
#include "core/logging/Logger.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace kb {

ApiClient &ApiClient::instance() {
    static ApiClient *inst = new ApiClient();
    return *inst;
}

ApiClient::ApiClient(QObject *parent) : QObject(parent) {}

QNetworkRequest ApiClient::buildRequest(const QString &path, bool jsonContentType) const {
    QUrl url(Settings::instance().baseUrl() + path);
    QNetworkRequest req(url);
    if (jsonContentType) {
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    }
    const QString tok = Session::instance().token();
    if (!tok.isEmpty()) {
        req.setRawHeader("Authorization", QByteArray("Bearer ") + tok.toUtf8());
    }
    return req;
}

void ApiClient::handle(QNetworkReply *reply, Callback cb) {
    m_activeReplies.insert(reply);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, cb]() {
        m_activeReplies.remove(reply);
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

        if (!res.ok && netErr != QNetworkReply::OperationCanceledError) {
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

void ApiClient::abortAll() {
    const auto pending = m_activeReplies;
    for (QNetworkReply *reply : pending) {
        if (reply) {
            reply->abort();
        }
    }
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

void ApiClient::upload(const QString &path, const QString &filePath, const QString &fieldName,
                       const QJsonObject &extraFields, Callback cb) {
    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // 文本字段
    for (auto it = extraFields.begin(); it != extraFields.end(); ++it) {
        QHttpPart textPart;
        textPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                           QStringLiteral("form-data; name=\"%1\"").arg(it.key()));
        const QJsonValue v = it.value();
        textPart.setBody((v.isString() ? v.toString()
                                       : QString::number(static_cast<qint64>(v.toDouble()))).toUtf8());
        multiPart->append(textPart);
    }

    // 文件字段
    auto *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        delete file;
        delete multiPart;
        ApiResponse res;
        res.networkError = true;
        res.message = QStringLiteral("无法打开文件：%1").arg(filePath);
        if (cb) {
            cb(res);
        }
        return;
    }
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QStringLiteral("form-data; name=\"%1\"; filename=\"%2\"")
                           .arg(fieldName, QFileInfo(filePath).fileName()));
    filePart.setBodyDevice(file);
    file->setParent(multiPart); // multiPart 析构时一并释放 file
    multiPart->append(filePart);

    QNetworkReply *reply = m_manager.post(buildRequest(path, false), multiPart);
    multiPart->setParent(reply); // reply 析构时释放 multiPart
    handle(reply, std::move(cb));
}

void ApiClient::download(const QString &path, const QString &savePath, DownloadCallback cb) {
    QNetworkReply *reply = m_manager.get(buildRequest(path, false));
    QObject::connect(reply, &QNetworkReply::finished, this, [reply, savePath, cb]() {
        const auto netErr = reply->error();
        if (netErr != QNetworkReply::NoError) {
            const QString err = reply->errorString();
            reply->deleteLater();
            if (cb) {
                cb(false, err);
            }
            return;
        }
        const QByteArray bytes = reply->readAll();
        reply->deleteLater();
        QFile out(savePath);
        if (!out.open(QIODevice::WriteOnly)) {
            if (cb) {
                cb(false, QStringLiteral("无法写入文件：%1").arg(savePath));
            }
            return;
        }
        out.write(bytes);
        out.close();
        if (cb) {
            cb(true, QString());
        }
    });
}

} // namespace kb
