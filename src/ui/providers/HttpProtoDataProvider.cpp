#include "HttpProtoDataProvider.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDebug>

HttpProtoDataProvider::HttpProtoDataProvider(const QUrl &url, QObject *parent)
    : IDataProvider(parent), m_url(url) {
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &HttpProtoDataProvider::onReplyFinished);
}

void HttpProtoDataProvider::fetchData() {
    QNetworkRequest req(m_url);
    req.setRawHeader("Accept", "application/x-protobuf");
    m_nam->get(req);
}

void HttpProtoDataProvider::onReplyFinished(QNetworkReply *reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(QStringLiteral("HTTP Proto %1: %2")
            .arg(reply->error()).arg(reply->errorString()));
        return;
    }
    const QByteArray body = reply->readAll();
    const TableData data = parseProto(body);
    if (!data.isValid()) {
        emit errorOccurred(QStringLiteral("Protobuf parse error: invalid data format"));
        return;
    }
    emit dataReady(data);
}

TableData HttpProtoDataProvider::parseProto(const QByteArray &data) const {
    table_data::TableData protoMsg;
    if (!protoMsg.ParseFromArray(data.constData(), data.size())) {
        qWarning("HttpProtoDataProvider: failed to parse protobuf data");
        return {};
    }
    return TableData::fromProto(protoMsg);
}
