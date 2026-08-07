#ifndef TABLE_DATA_SERVER_H
#define TABLE_DATA_SERVER_H

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <memory>
#include <thread>
#include <atomic>

namespace httplib { class Server; }

/**
 * @brief HTTP 表格数据服务器 — 基于 cpp-httplib
 *
 * cpp-httplib 内部使用 poll/epoll + 线程池，自动处理:
 *   Content-Length、分块传输、TCP 粘包/拆包、
 *   Keep-Alive、并发连接等 HTTP 协议细节。
 *
 * 接口:
 *   GET /api/table       → JSON
 *   GET /api/table/proto → Protobuf 二进制 (TABLE_DATA_SERVER_PROTO)
 *   GET /health          → 健康检查
 */
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
    void requestReceived(const QString &method, const QString &path,
                         const QString &clientIp);
    void errorOccurred(const QString &message);

private:
    void registerRoutes();
#ifdef TABLE_DATA_SERVER_PROTO
    QByteArray serializeToProto() const;
#endif

    std::unique_ptr<httplib::Server> m_server;
    std::thread                      m_thread;
    std::atomic<bool>                m_running{false};
    quint16     m_port = 0;
    QJsonObject m_tableData;
};

#endif
