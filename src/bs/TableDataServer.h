#ifndef TABLE_DATA_SERVER_H
#define TABLE_DATA_SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QTimer>

class TableDataServer : public QObject {
    Q_OBJECT

public:
    explicit TableDataServer(QObject *parent = nullptr);
    ~TableDataServer() override;

    bool start(quint16 port = 8080);
    void stop();
    quint16 port() const { return m_port; }
    bool isRunning() const;

    void setTableData(const QJsonObject &data) { m_tableData = data; }
    QJsonObject tableData() const { return m_tableData; }

signals:
    void started(quint16 port);
    void stopped();
    void requestReceived(const QString &method, const QString &path, const QString &clientIp);
    void errorOccurred(const QString &message);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    void handleRequest(QTcpSocket *socket, const QByteArray &raw);
    void sendResponse(QTcpSocket *socket, int statusCode,
                      const QString &contentType, const QByteArray &body);
    void sendJson(QTcpSocket *socket, int statusCode, const QJsonObject &json);
    void sendError(QTcpSocket *socket, int statusCode, const QString &message);
#ifdef TABLE_DATA_SERVER_PROTO
    QByteArray serializeToProto() const;
#endif

    QTcpServer  *m_server  = nullptr;
    quint16      m_port    = 0;
    QJsonObject  m_tableData;
    QSet<QTcpSocket *> m_clients;
};

#endif
