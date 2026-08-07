#include "TableDataServer.h"

#include <QDateTime>
#include <QJsonDocument>

#include <httplib/httplib.h>

#ifdef TABLE_DATA_SERVER_PROTO
#include "table_data.pb.h"
#endif

// ============================================================
//  构造 / 析构
// ============================================================

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
}

TableDataServer::~TableDataServer() { stop(); }

// ============================================================
//  启动 / 停止
// ============================================================

bool TableDataServer::start(quint16 port) {
    if (m_running) { return true; }

    m_server = std::make_unique<httplib::Server>();
    registerRoutes();
    m_port = port;

    // std::thread 运行阻塞的 listen()，主线程继续 QCoreApplication 事件循环
    m_running = true;
    m_thread = std::thread([this, port]() {
        m_server->listen("0.0.0.0", static_cast<int>(port));
        m_running = false;
    });

    // 等待 listen() 就绪
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    if (!m_server->is_running()) {
        m_running = false;
        if (m_thread.joinable()) m_thread.join();
        emit errorOccurred(QStringLiteral("Failed to listen on port %1").arg(port));
        return false;
    }

    emit started(m_port);
    return true;
}

void TableDataServer::stop() {
    if (m_server && m_server->is_running()) {
        m_server->stop();
    }
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_server.reset();
    emit stopped();
}

bool TableDataServer::isRunning() const {
    return m_running && m_server && m_server->is_running();
}

// ============================================================
//  路由注册
// ============================================================

void TableDataServer::registerRoutes() {
    // ---- GET /api/table (JSON) ----
    m_server->Get("/api/table", [this](const httplib::Request &req,
                                        httplib::Response &res) {
        emit requestReceived(
            QStringLiteral("GET"),
            QStringLiteral("/api/table"),
            QString::fromStdString(req.remote_addr));

        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(
            QJsonDocument(m_tableData).toJson(QJsonDocument::Indented).toStdString(),
            "application/json; charset=utf-8");
    });

    // ---- GET /api/table/proto (Protobuf) ----
#ifdef TABLE_DATA_SERVER_PROTO
    m_server->Get("/api/table/proto", [this](const httplib::Request &req,
                                              httplib::Response &res) {
        emit requestReceived(
            QStringLiteral("GET"),
            QStringLiteral("/api/table/proto"),
            QString::fromStdString(req.remote_addr));

        const QByteArray pb = serializeToProto();
        if (pb.isEmpty()) {
            res.status = 500;
            res.set_content(R"({"error":"Protobuf serialization failed"})",
                            "application/json");
            return;
        }
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(pb.toStdString(), "application/x-protobuf");
    });
#endif

    // ---- GET /health ----
    m_server->Get("/health", [this](const httplib::Request &req,
                                     httplib::Response &res) {
        emit requestReceived(
            QStringLiteral("GET"),
            QStringLiteral("/health"),
            QString::fromStdString(req.remote_addr));

        QJsonObject h;
        h[QStringLiteral("status")] = QStringLiteral("ok");
        h[QStringLiteral("time")]   = QDateTime::currentDateTimeUtc()
                                       .toString(Qt::ISODate);
        h[QStringLiteral("rows")]   = m_tableData
                                       .value(QStringLiteral("rows"))
                                       .toArray().size();

        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(
            QJsonDocument(h).toJson(QJsonDocument::Indented).toStdString(),
            "application/json; charset=utf-8");
    });
}

// ============================================================
//  Protobuf 序列化
// ============================================================

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
        for (const auto &cv : rv.toArray())
            row->add_cells(cv.toString().toStdString());
    }
    std::string out;
    if (!msg.SerializeToString(&out)) return {};
    return QByteArray::fromStdString(out);
}
#endif
