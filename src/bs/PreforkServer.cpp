#include "PreforkServer.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include <spdlog/spdlog.h>

#include "logutil.h"

// ============================================================
//  响应数据（fork 之前构造，worker 靠写时复制共享）
// ============================================================

namespace {

// 与 TableDataServer 构造中硬编码的数据保持一致
const std::string kTableJson = R"JSON({
    "columns": ["姓名", "年龄", "部门", "职位", "入职日期", "备注"],
    "editable": [true, true, false, true, true, true],
    "rows": [
        ["HappyWealthy", "28", "研发部", "高级工程师", "2022-03-15", "技术骨干"],
        ["李四", "35", "产品部", "产品经理", "2021-07-01", ""],
        ["王五", "24", "设计部", "UI 设计师", "2023-01-10", "实习生转正"],
        ["赵六", "42", "管理层", "技术总监", "2018-05-20", "部门负责人"],
        ["孙七", "30", "测试部", "测试工程师", "2022-09-01", ""],
        ["周八", "26", "运维部", "运维工程师", "2023-06-01", ""],
        ["吴九", "33", "市场部", "市场经理", "2020-11-15", "部门骨干"],
        ["郑十", "29", "研发部", "前端工程师", "2024-01-10", ""]
    ]
}
)JSON";

const std::string kHealthJson = R"({"status":"ok","rows":8})";

// 信号标志（sig_atomic_t 保证在 handler 与主循环间安全读写）
volatile sig_atomic_t g_stop        = 0;
volatile sig_atomic_t g_childExited = 0;

void onSigterm(int) { g_stop = 1; }
void onSigchld(int)  { g_childExited = 1; }

// 发送整个字符串（处理 write 的短写与 EINTR）
void sendAll(int fd, const std::string& data) {
    size_t off = 0;
    while (off < data.size()) {
        ssize_t w = write(fd, data.data() + off, data.size() - off);
        if (w < 0) {
            if (errno == EINTR) continue;
            return;
        }
        off += static_cast<size_t>(w);
    }
}

// SIGCHLD 会中断阻塞的 waitpid（返回 -1/EINTR），必须重试，否则漏回收子进程
pid_t waitpidRetry(pid_t pid, int* status, int options) {
    pid_t r;
    do {
        r = waitpid(pid, status, options);
    } while (r < 0 && errno == EINTR);
    return r;
}

// worker 独立日志 fd（O_APPEND，短写原子；不复用父进程 spdlog rotating sink）
int g_workerLogFd = -1;

void openWorkerLogFile(pid_t pid) {
    const std::string path =
        LogUtil::resolveLogDirectory() + "/table_data_server_prefork_worker_"
        + std::to_string(pid) + ".log";
    g_workerLogFd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
    if (g_workerLogFd < 0) {
        const std::string err = "[worker " + std::to_string(pid)
            + "] open log failed: " + path + " — " + strerror(errno) + "\n";
        write(STDERR_FILENO, err.data(), err.size());
    }
}

// 单次 write 到 stderr + 本 worker 日志文件，避免多进程共享 rotating sink
void workerLog(const std::string& msg) {
    write(STDERR_FILENO, msg.data(), msg.size());
    if (g_workerLogFd >= 0) {
        // 忽略短写/错误：日志失败不应影响 accept 循环
        (void)write(g_workerLogFd, msg.data(), msg.size());
    }
}

} // namespace

// ============================================================
//  构造 / 析构
// ============================================================

PreforkServer::~PreforkServer() { stop(); }

// ============================================================
//  启动 / 运行 / 停止
// ============================================================

bool PreforkServer::start(uint16_t port, int numWorkers) {
    if (m_running) return true;

    installSignals();
    if (!createListenSocket(port)) return false;

    forkWorkers(numWorkers);
    m_running = true;
    return true;
}

void PreforkServer::run() {
    while (!g_stop) {
        pause();  // 等待任意信号（SIGINT/SIGTERM 置 g_stop，SIGCHLD 置 g_childExited）
        if (g_childExited) {
            g_childExited = 0;
            reapChildren();
        }
    }
}

void PreforkServer::stop() {
    if (!m_running) return;

    // 1. 向所有 worker 发 SIGTERM，通知其退出
    for (pid_t pid : m_children) {
        kill(pid, SIGTERM);
    }
    // 2. 阻塞等待并回收每个 worker（若已被 reapChildren 回收则跳过）
    for (pid_t pid : m_children) {
        int status = 0;
        if (waitpidRetry(pid, &status, 0) > 0) {
            spdlog::info("worker {} exited, status={}", pid, WEXITSTATUS(status));
        }
    }
    m_children.clear();

    if (m_listenFd >= 0) {
        close(m_listenFd);
        m_listenFd = -1;
    }
    m_running = false;
}

// ============================================================
//  socket / fork
// ============================================================

bool PreforkServer::createListenSocket(uint16_t port) {
    m_listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenFd < 0) {
        spdlog::error("socket() failed: {}", strerror(errno));
        return false;
    }

    int opt = 1;
    setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 0.0.0.0
    addr.sin_port        = htons(port);

    if (bind(m_listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        spdlog::error("bind() on port {} failed: {}", port, strerror(errno));
        close(m_listenFd);
        m_listenFd = -1;
        return false;
    }
    if (listen(m_listenFd, 128) < 0) {
        spdlog::error("listen() failed: {}", strerror(errno));
        close(m_listenFd);
        m_listenFd = -1;
        return false;
    }
    return true;
}

void PreforkServer::forkWorkers(int numWorkers) {
    for (int i = 0; i < numWorkers; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            spdlog::error("fork() failed: {}", strerror(errno));
            continue;
        }
        if (pid == 0) {
            // 子进程：进入 worker 循环，永不返回
            workerLoop();
            _exit(0);  // 使用 _exit 避免运行 atexit/spdlog flush
        }
        m_children.push_back(pid);
        spdlog::info("forked worker pid={}", pid);
    }
}

void PreforkServer::workerLoop() {
    const pid_t pid = getpid();
    // 子进程独立文件日志，不复用父进程 spdlog（rotating file sink 跨进程不安全）
    openWorkerLogFile(pid);
    workerLog("[worker " + std::to_string(pid) + "] started\n");

    while (!g_stop) {
        int client = accept(m_listenFd, nullptr, nullptr);
        if (client < 0) {
            if (errno == EINTR) {
                // accept 被信号中断：若收到停止信号则退出，否则继续
                if (g_stop) break;
                continue;
            }
            workerLog("[worker " + std::to_string(pid) + "] accept: "
                      + strerror(errno) + "\n");
            usleep(50 * 1000);
            continue;
        }
        handleClient(client);
        close(client);
    }
    workerLog("[worker " + std::to_string(pid) + "] exiting\n");
}

// ============================================================
//  HTTP 处理（最小实现，仅 GET 两个端点）
// ============================================================

void PreforkServer::handleClient(int fd) {
    char buf[4096];
    const ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n <= 0) return;
    buf[n] = '\0';

    // 解析请求行: "GET /api/table HTTP/1.1"
    std::string reqLine(buf);
    const size_t lineEnd = reqLine.find("\r\n");
    if (lineEnd != std::string::npos) reqLine = reqLine.substr(0, lineEnd);

    std::istringstream iss(reqLine);
    std::string method, path, version;
    iss >> method >> path >> version;

    workerLog("[worker " + std::to_string(getpid()) + "] "
              + method + " " + path + "\n");

    std::string body;
    int status = 200;
    if (path == "/api/table") {
        body = kTableJson;
    } else if (path == "/health") {
        body = kHealthJson;
    } else {
        status = 404;
        body = R"({"error":"not found"})";
    }

    const char* reason = (status == 200) ? "OK" : "Not Found";
    std::ostringstream head;
    head << "HTTP/1.1 " << status << " " << reason << "\r\n"
         << "Content-Type: application/json; charset=utf-8\r\n"
         << "Content-Length: " << body.size() << "\r\n"
         << "Connection: close\r\n"
         << "\r\n";

    sendAll(fd, head.str());
    sendAll(fd, body);
}

// ============================================================
//  子进程回收 / 信号
// ============================================================

void PreforkServer::reapChildren() {
    int status = 0;
    pid_t pid;
    // WNOHANG 非阻塞地回收所有已退出子进程，避免僵尸
    while ((pid = waitpidRetry(-1, &status, WNOHANG)) > 0) {
        spdlog::warn("worker {} exited unexpectedly, status={}", pid, WEXITSTATUS(status));
    }
}

void PreforkServer::installSignals() {
    // 注意：不设置 SA_RESTART，使阻塞的 accept() 被信号中断并返回 EINTR，
    // 这样 worker 才能及时退出。
    struct sigaction sa{};
    sa.sa_handler = onSigterm;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT,  &sa, nullptr);

    struct sigaction chld{};
    chld.sa_handler = onSigchld;
    sigemptyset(&chld.sa_mask);
    sigaction(SIGCHLD, &chld, nullptr);
}
