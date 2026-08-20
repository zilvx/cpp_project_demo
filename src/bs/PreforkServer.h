#ifndef PREFORK_SERVER_H
#define PREFORK_SERVER_H

#include <sys/types.h>
#include <vector>
#include <atomic>

/**
 * @brief Prefork 多进程 HTTP 服务器（纯 POSIX，练习多进程）
 *
 * 与 cpp-httplib 版 TableDataServer 不同，本类自己管理 socket 与子进程：
 *
 *   主进程: socket/bind/listen → fork() N 个 worker → waitpid 回收
 *   worker : 继承 listen fd，各自 accept() 处理请求（写时复制共享响应数据）
 *
 * 关键点（fork 与 Qt 的坑）:
 *   - fork() 之后子进程不能再碰 Qt 对象（Qt 官方仅允许 fork 后立即 exec
 *     或执行 async-signal-safe 操作）。因此 worker 全程只用 POSIX API，
 *     响应数据在 fork 之前就序列化为 std::string，worker 靠写时复制共享。
 *   - worker 的日志只写 stderr，不复用父进程的 spdlog（rotating file sink
 *     在 fork 后跨进程共享文件偏移会互相覆盖）。
 *
 * 仅实现两个 JSON 端点: GET /api/table、GET /health。
 */
class PreforkServer {
public:
    PreforkServer() = default;
    ~PreforkServer();

    PreforkServer(const PreforkServer&) = delete;
    PreforkServer& operator=(const PreforkServer&) = delete;

    /// 创建监听 socket 并 fork 出 numWorkers 个 worker
    bool start(uint16_t port, int numWorkers);

    /// 主进程阻塞等待停止信号（SIGINT/SIGTERM）
    void run();

    /// 向所有 worker 发 SIGTERM 并 waitpid 回收
    void stop();

    bool isRunning() const { return m_running; }
    int  workerCount() const { return static_cast<int>(m_children.size()); }

private:
    bool createListenSocket(uint16_t port);
    void forkWorkers(int numWorkers);
    void workerLoop();          // 子进程入口，不返回
    void handleClient(int fd);
    void reapChildren();        // waitpid(-1, WNOHANG) 回收僵尸
    static void installSignals();

    int  m_listenFd = -1;
    std::vector<pid_t> m_children;
    std::atomic<bool>  m_running{false};
};

#endif // PREFORK_SERVER_H
