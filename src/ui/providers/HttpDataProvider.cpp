#include "HttpDataProvider.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <spdlog/spdlog.h>

HttpDataProvider::HttpDataProvider(const QUrl &url, QObject *parent)
    : IDataProvider(parent), m_url(url) {
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &HttpDataProvider::onReplyFinished);
}

void HttpDataProvider::fetchData() {
    QNetworkRequest req(m_url);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/json"));
    req.setRawHeader("Accept", "application/json");
    m_nam->get(req);
}

void HttpDataProvider::onReplyFinished(QNetworkReply *reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(QStringLiteral("HTTP %1: %2")
            .arg(reply->error()).arg(reply->errorString()));
        return;
    }
    const QByteArray body = reply->readAll();
    const TableData data = parseJson(body);
    if (!data.isValid()) {
        emit errorOccurred(QStringLiteral("JSON parse error: invalid data format"));
        return;
    }
    emit dataReady(data);
}

TableData HttpDataProvider::parseJson(const QByteArray &json) const {
    TableData result;
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError) {
        spdlog::warn("HttpDataProvider: JSON parse error at offset {}: {}",
                     err.offset, err.errorString().toStdString());
        return result;
    }
    const QJsonObject root = doc.object();
    const QJsonArray colArr = root.value(QStringLiteral("columns")).toArray();
    for (const auto &v : colArr) result.columns.append(v.toString());
    const QJsonArray rowArr = root.value(QStringLiteral("rows")).toArray();
    for (const auto &rv : rowArr) {
        const QJsonArray cellArr = rv.toArray();
        QStringList row;
        for (const auto &cv : cellArr) row.append(cv.toString());
        result.rows.append(row);
    }
    const QJsonArray editArr = root.value(QStringLiteral("editable")).toArray();
    if (!editArr.isEmpty())
        for (const auto &v : editArr) result.editable.append(v.toBool());
    return result;
}
