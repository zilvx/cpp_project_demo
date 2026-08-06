#include <QCoreApplication>
#include <iostream>
#include "TableDataServer.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    auto *server = new TableDataServer(&app);

    QObject::connect(server, &TableDataServer::started, [](quint16 port) {
        std::cout << "\n======================================\n"
                  << "  Table Data Server — Running\n"
                  << "======================================\n"
                  << "  GET http://localhost:" << port << "/api/table\n"
                  << "  GET http://localhost:" << port << "/health\n"
#ifdef TABLE_DATA_SERVER_PROTO
                  << "  GET http://localhost:" << port << "/api/table/proto\n"
#endif
                  << "======================================\n"
                  << "  Press Ctrl+C to stop\n\n";
    });

    QObject::connect(server, &TableDataServer::errorOccurred,
                     [](const QString &msg) { std::cerr << "[ERROR] " << msg.toStdString() << std::endl; });

    QObject::connect(server, &TableDataServer::requestReceived,
                     [](const QString &method, const QString &path, const QString &ip) {
        qDebug().noquote() << QStringLiteral("[%1] %2 %3").arg(ip, method, path);
    });

    const quint16 port = (argc > 1) ? QString::fromLocal8Bit(argv[1]).toUShort() : 8080;
    if (!server->start(port)) {
        std::cerr << "Failed to start server on port " << port << std::endl;
        return 1;
    }
    return app.exec();
}
