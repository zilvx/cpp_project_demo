#ifdef USE_PROTO_DATA
#ifndef HTTP_PROTO_DATA_PROVIDER_H
#define HTTP_PROTO_DATA_PROVIDER_H

#include "IDataProvider.h"
#include <QNetworkAccessManager>
#include <QUrl>

class HttpProtoDataProvider : public IDataProvider {
    Q_OBJECT
public:
    explicit HttpProtoDataProvider(const QUrl &url, QObject *parent = nullptr);
    void fetchData() override;
private slots:
    void onReplyFinished(QNetworkReply *reply);
private:
    TableData parseProto(const QByteArray &data) const;
    QNetworkAccessManager *m_nam;
    QUrl m_url;
};

#endif
#endif
