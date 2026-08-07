#include <QCoreApplication>
#include "../utils/logging/logutil.h"
#include "TableDataServer.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    LogUtil::init("table_data_server");

    auto *server = new TableDataServer(&app);

    QObject::connect(server, &TableDataServer::started, [](quint16 port) {
        spdlog::info("======================================");
        spdlog::info("  Table Data Server — Running");
        spdlog::info("======================================");
        spdlog::info("  GET http://localhost:{}/api/table", port);
        spdlog::info("  GET http://localhost:{}/health", port);
#ifdef TABLE_DATA_SERVER_PROTO
        spdlog::info("  GET http://localhost:{}/api/table/proto", port);
#endif
        spdlog::info("======================================");
        spdlog::info("  Press Ctrl+C to stop");
    });

    QObject::connect(server, &TableDataServer::errorOccurred,
                     [](const QString &msg) { spdlog::error("{}", msg.toStdString()); });

    QObject::connect(server, &TableDataServer::requestReceived,
                     [](const QString &method, const QString &path, const QString &ip) {
        spdlog::info("[{}] {} {}", ip.toStdString(), method.toStdString(), path.toStdString());
    });

    const quint16 port = (argc > 1) ? QString::fromLocal8Bit(argv[1]).toUShort() : 8080;
    if (!server->start(port)) {
        spdlog::error("Failed to start server on port {}", port);
        LogUtil::shutdown();
        return 1;
    }

    const int ret = app.exec();
    LogUtil::shutdown();
    return ret;
}
