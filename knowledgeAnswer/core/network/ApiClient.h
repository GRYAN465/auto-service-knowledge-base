#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

#include <functional>

class QNetworkReply;

namespace kb {

/** 一次 REST 调用的结果（已解析统一响应体 {code,message,data}）。 */
struct ApiResponse {
    bool ok = false;          // 业务成功：拿到结构化响应且 code==0
    bool networkError = false; // 传输层失败（连不上/超时），无可解析响应体
    int code = -1;
    QString message;
    QJsonValue data;          // 业务负载，可能是对象或数组

    QJsonObject object() const { return data.toObject(); }
};

/**
 * REST 客户端（单例）：统一 baseUrl、注入 Authorization: Bearer、解析统一响应体。
 * 异步，结果经回调返回（在主线程/事件循环回调）。基于 QNetworkAccessManager。
 */
class ApiClient : public QObject {
    Q_OBJECT
public:
    using Callback = std::function<void(const ApiResponse &)>;
    using DownloadCallback = std::function<void(bool ok, const QString &error)>;

    static ApiClient &instance();

    void get(const QString &path, Callback cb);
    void post(const QString &path, const QJsonObject &body, Callback cb);
    void put(const QString &path, const QJsonObject &body, Callback cb);
    void del(const QString &path, Callback cb);

    /**
     * multipart/form-data 上传单个文件（外加可选文本字段）。解析统一响应体（同 JSON 接口）。
     * fieldName 为文件表单字段名（后端 @RequestParam("file")）。
     */
    void upload(const QString &path, const QString &filePath, const QString &fieldName,
                const QJsonObject &extraFields, Callback cb);

    /** 二进制下载并写入 savePath（不走 JSON 解析）。自动注入 Authorization。 */
    void download(const QString &path, const QString &savePath, DownloadCallback cb);

private:
    explicit ApiClient(QObject *parent = nullptr);

    QNetworkRequest buildRequest(const QString &path, bool jsonContentType = true) const;
    void handle(QNetworkReply *reply, Callback cb);

    QNetworkAccessManager m_manager;
};

} // namespace kb
