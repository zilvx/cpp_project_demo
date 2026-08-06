#ifndef HTTP_DATA_PROVIDER_H
#define HTTP_DATA_PROVIDER_H

#include "IDataProvider.h"
#include <QNetworkAccessManager>
#include <QUrl>

class HttpDataProvider : public IDataProvider {
    Q_OBJECT
public:
    explicit HttpDataProvider(const QUrl &url, QObject *parent = nullptr);
    void fetchData() override;
private slots:
    void onReplyFinished(QNetworkReply *reply);
private:
    TableData parseJson(const QByteArray &json) const;
    QNetworkAccessManager *m_nam;
    QUrl m_url;
};

#endif
