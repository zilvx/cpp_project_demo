#include <unistd.h>

#include <cstdlib>
#include <iostream>

#include <spdlog/spdlog.h>

#include "../utils/logging/logutil.h"
#include "PreforkServer.h"

int main(int argc, char* argv[]) {
    LogUtil::init("table_data_server_prefork");

    uint16_t port    = 8080;
    int      workers = 4;

    // 解析命令行: table_data_server_prefork [-p PORT] [-w WORKERS] [PORT]
    int opt;
    while ((opt = getopt(argc, argv, "p:w:h")) != -1) {
        switch (opt) {
        case 'p': port    = static_cast<uint16_t>(std::atoi(optarg)); break;
        case 'w': workers = std::atoi(optarg); break;
        case 'h':
        default:
            std::cout << "用法: table_data_server_prefork [-p PORT] [-w WORKERS] [PORT]\n"
                      << "  -p PORT    监听端口 (默认 8080)\n"
                      << "  -w WORKERS fork 的 worker 数 (默认 4)\n";
            LogUtil::shutdown();
            return (opt == 'h') ? 0 : 1;
        }
    }
    if (optind < argc) port = static_cast<uint16_t>(std::atoi(argv[optind]));

    PreforkServer server;
    if (!server.start(port, workers)) {
        spdlog::error("Failed to start prefork server on port {}", port);
        LogUtil::shutdown();
        return 1;
    }

    spdlog::info("Prefork server listening on 0.0.0.0:{} with {} workers", port, workers);
    spdlog::info("  GET http://localhost:{}/api/table", port);
    spdlog::info("  GET http://localhost:{}/health", port);
    spdlog::info("Press Ctrl+C to stop");

    server.run();   // 阻塞直到 SIGINT/SIGTERM
    server.stop();  // 通知并回收所有 worker

    spdlog::info("Prefork server stopped");
    LogUtil::shutdown();
    return 0;
}
