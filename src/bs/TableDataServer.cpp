#include "TableDataServer.h"
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#ifdef TABLE_DATA_SERVER_PROTO
#include "table_data.pb.h"
#endif

TableDataServer::TableDataServer(QObject *parent) : QObject(parent) {
    m_tableData = QJsonObject::fromVariantMap({
        {QStringLiteral("columns"), QJsonArray({
            QStringLiteral("姓名"), QStringLiteral("年龄"), QStringLiteral("部门"),
            QStringLiteral("职位"), QStringLiteral("入职日期"), QStringLiteral("备注"),
        })},
        {QStringLiteral("editable"), QJsonArray({true, true, false, true, true, true})},
        {QStringLiteral("rows"), QJsonArray({
            QJsonArray({QStringLiteral("HappyWealthy"), QStringLiteral("28"), QStringLiteral("研发部"),
                        QStringLiteral("高级工程师"), QStringLiteral("2022-03-15"), QStringLiteral("技术骨干")}),
            QJsonArray({QStringLiteral("李四"), QStringLiteral("35"), QStringLiteral("产品部"),
                        QStringLiteral("产品经理"), QStringLiteral("2021-07-01"), QStringLiteral("")}),
            QJsonArray({QStringLiteral("王五"), QStringLiteral("24"), QStringLiteral("设计部"),
                        QStringLiteral("UI 设计师"), QStringLiteral("2023-01-10"), QStringLiteral("实习生转正")}),
            QJsonArray({QStringLiteral("赵六"), QStringLiteral("42"), QStringLiteral("管理层"),
                        QStringLiteral("技术总监"), QStringLiteral("2018-05-20"), QStringLiteral("部门负责人")}),
            QJsonArray({QStringLiteral("孙七"), QStringLiteral("30"), QStringLiteral("测试部"),
                        QStringLiteral("测试工程师"), QStringLiteral("2022-09-01"), QStringLiteral("")}),
            QJsonArray({QStringLiteral("周八"), QStringLiteral("26"), QStringLiteral("运维部"),
                        QStringLiteral("运维工程师"), QStringLiteral("2023-06-01"), QStringLiteral("")}),
            QJsonArray({QStringLiteral("吴九"), QStringLiteral("33"), QStringLiteral("市场部"),
                        QStringLiteral("市场经理"), QStringLiteral("2020-11-15"), QStringLiteral("部门骨干")}),
            QJsonArray({QStringLiteral("郑十"), QStringLiteral("29"), QStringLiteral("研发部"),
                        QStringLiteral("前端工程师"), QStringLiteral("2024-01-10"), QStringLiteral("")}),
        })},
    });
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &TableDataServer::onNewConnection);
}

TableDataServer::~TableDataServer() { stop(); }

bool TableDataServer::start(quint16 port) {
    if (m_server->isListening()) { return true; }
    if (!m_server->listen(QHostAddress::Any, port)) {
        emit errorOccurred(QStringLiteral("Failed to listen on port %1: %2")
            .arg(port).arg(m_server->errorString()));
        return false;
    }
    m_port = port;
    emit started(port);
    return true;
}

void TableDataServer::stop() {
    for (auto *c : m_clients) { c->disconnectFromHost(); c->deleteLater(); }
    m_clients.clear();
    if (m_server->isListening()) { m_server->close(); emit stopped(); }
}

bool TableDataServer::isRunning() const { return m_server && m_server->isListening(); }

void TableDataServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        auto *c = m_server->nextPendingConnection();
        m_clients.insert(c);
        connect(c, &QTcpSocket::readyRead, this, &TableDataServer::onReadyRead);
        connect(c, &QTcpSocket::disconnected, this, &TableDataServer::onDisconnected);
    }
}

void TableDataServer::onDisconnected() {
    auto *c = qobject_cast<QTcpSocket *>(sender());
    if (c) { m_clients.remove(c); c->deleteLater(); }
}

void TableDataServer::onReadyRead() {
    auto *c = qobject_cast<QTcpSocket *>(sender());
    if (c) handleRequest(c, c->readAll());
}

void TableDataServer::handleRequest(QTcpSocket *socket, const QByteArray &raw) {
    const int nl = raw.indexOf('\n');
    if (nl < 0) { sendError(socket, 400, QStringLiteral("Bad Request")); return; }
    const QString reqLine = QString::fromUtf8(raw.left(nl)).trimmed();
    const QStringList parts = reqLine.split(' ');
    if (parts.size() < 2) { sendError(socket, 400, QStringLiteral("Bad Request")); return; }
    const QString method = parts[0];
    QString path = parts[1];
    const int qi = path.indexOf('?');
    if (qi >= 0) path = path.left(qi);

    emit requestReceived(method, path, socket->peerAddress().toString());
    if (method != QStringLiteral("GET")) { sendError(socket, 405, QStringLiteral("Method Not Allowed")); return; }

    if (path == QStringLiteral("/api/table")) {
        sendJson(socket, 200, m_tableData);
    }
#ifdef TABLE_DATA_SERVER_PROTO
    else if (path == QStringLiteral("/api/table/proto")) {
        const QByteArray pb = serializeToProto();
        if (pb.isEmpty()) { sendError(socket, 500, QStringLiteral("Protobuf serialization failed")); return; }
        sendResponse(socket, 200, QStringLiteral("application/x-protobuf"), pb);
    }
#endif
    else if (path == QStringLiteral("/health")) {
        QJsonObject h;
        h[QStringLiteral("status")] = QStringLiteral("ok");
        h[QStringLiteral("time")] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        h[QStringLiteral("rows")] = m_tableData.value(QStringLiteral("rows")).toArray().size();
        sendJson(socket, 200, h);
    } else {
        sendError(socket, 404, QStringLiteral("Not Found"));
    }
}

void TableDataServer::sendResponse(QTcpSocket *s, int code, const QString &ct, const QByteArray &body) {
    const QByteArray statusText = (code == 200) ? QByteArray("OK") :
        (code == 400) ? QByteArray("Bad Request") : (code == 404) ? QByteArray("Not Found") :
        (code == 405) ? QByteArray("Method Not Allowed") : (code == 500) ? QByteArray("Internal Server Error") : QByteArray("Unknown");
    const QByteArray resp = QByteArray("HTTP/1.1 ") + QByteArray::number(code) + " " + statusText + "\r\n"
        + "Content-Type: " + ct.toUtf8() + "; charset=utf-8\r\n"
        + "Content-Length: " + QByteArray::number(body.size()) + "\r\n"
        + "Access-Control-Allow-Origin: *\r\n"
        + "Access-Control-Allow-Methods: GET, OPTIONS\r\n"
        + "Access-Control-Allow-Headers: Content-Type, Accept\r\n"
        + "Connection: close\r\n"
        + "Server: TableDataServer/1.0\r\n\r\n" + body;
    s->write(resp); s->flush(); s->disconnectFromHost();
}

void TableDataServer::sendJson(QTcpSocket *s, int code, const QJsonObject &json) {
    sendResponse(s, code, QStringLiteral("application/json"), QJsonDocument(json).toJson(QJsonDocument::Indented));
}

void TableDataServer::sendError(QTcpSocket *s, int code, const QString &msg) {
    QJsonObject err;
    err[QStringLiteral("error")] = msg;
    err[QStringLiteral("code")] = code;
    sendJson(s, code, err);
}

#ifdef TABLE_DATA_SERVER_PROTO
QByteArray TableDataServer::serializeToProto() const {
    table_data::TableData msg;
    const QJsonArray ca = m_tableData.value(QStringLiteral("columns")).toArray();
    for (const auto &v : ca) msg.add_columns(v.toString().toStdString());
    const QJsonArray ea = m_tableData.value(QStringLiteral("editable")).toArray();
    for (const auto &v : ea) msg.add_editable(v.toBool());
    const QJsonArray ra = m_tableData.value(QStringLiteral("rows")).toArray();
    for (const auto &rv : ra) {
        auto *row = msg.add_rows();
        for (const auto &cv : rv.toArray()) row->add_cells(cv.toString().toStdString());
    }
    std::string out;
    if (!msg.SerializeToString(&out)) return {};
    return QByteArray::fromStdString(out);
}
#endif
