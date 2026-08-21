#include "logutil.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#if defined(__APPLE__)
#include <limits.h>
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {

fs::path executablePath() {
#if defined(__APPLE__)
    char buf[PATH_MAX];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0) {
        std::error_code ec;
        const fs::path p = fs::weakly_canonical(buf, ec);
        return ec ? fs::path(buf) : p;
    }
#elif defined(__linux__)
    char buf[PATH_MAX];
    const ssize_t n = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        std::error_code ec;
        const fs::path p = fs::weakly_canonical(buf, ec);
        return ec ? fs::path(buf) : p;
    }
#elif defined(_WIN32)
    char buf[MAX_PATH];
    const DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
        std::error_code ec;
        const fs::path p = fs::weakly_canonical(buf, ec);
        return ec ? fs::path(buf) : p;
    }
#endif
    return {};
}

bool isProjectRoot(const fs::path& dir) {
    std::error_code ec;
    if (!fs::is_directory(dir, ec))
        return false;
    // 源码树特征：CMakeLists.txt / .git / 已有 logs/
    if (fs::exists(dir / "CMakeLists.txt", ec))
        return true;
    if (fs::exists(dir / ".git", ec))
        return true;
    if (fs::is_directory(dir / "logs", ec) && fs::exists(dir / "AGENTS.md", ec))
        return true;
    return false;
}

/// 从 start 向上最多 max_up 层，寻找项目根（统一归档到 <root>/logs）
fs::path findProjectRoot(const fs::path& start, int max_up = 8) {
    std::error_code ec;
    fs::path cur = start;
    if (cur.empty())
        return {};
    if (fs::is_regular_file(cur, ec))
        cur = cur.parent_path();

    for (int i = 0; i <= max_up && !cur.empty(); ++i) {
        if (isProjectRoot(cur))
            return cur;
        const fs::path parent = cur.parent_path();
        if (parent == cur)
            break;
        cur = parent;
    }
    return {};
}

/// 可执行文件所在“逻辑目录”：.app 包取 bundle 父目录，否则取 exe 父目录
fs::path executableBaseDir() {
    const fs::path exe = executablePath();
    if (exe.empty())
        return {};

    fs::path dir = exe.parent_path();
    // .../Foo.app/Contents/MacOS → 从 bundle 父目录开始向上找项目根
    if (dir.filename() == "MacOS" && dir.parent_path().filename() == "Contents") {
        const fs::path bundle = dir.parent_path().parent_path();  // Foo.app
        return bundle.parent_path();
    }
    return dir;
}

/// 统一候选：优先项目根 logs/，保证 build/*.app / build/xxx 都写到仓库 logs/
fs::path logDirectoryCandidate() {
    // 1) 从可执行文件位置向上找项目根
    if (const fs::path base = executableBaseDir(); !base.empty()) {
        if (const fs::path root = findProjectRoot(base); !root.empty())
            return root / "logs";
    }
    // 2) 从 CWD 向上找（在仓库内 cmake/ctest/脚本启动时）
    if (const fs::path root = findProjectRoot(fs::current_path()); !root.empty())
        return root / "logs";

    // 3) 找不到项目根时：仍尽量用 exe 旁或 CWD 下的 logs/
    if (const fs::path base = executableBaseDir(); !base.empty())
        return base / "logs";
    return fs::current_path() / "logs";
}

bool ensureDirectory(const fs::path& dir, std::string& err) {
    std::error_code ec;
    if (fs::exists(dir, ec) && fs::is_directory(dir, ec))
        return true;
    if (!fs::create_directories(dir, ec) || ec) {
        err = "create_directories(" + dir.string() + ") failed: " + ec.message();
        return false;
    }
    return true;
}

void reportFailure(const std::string& msg) {
    std::cerr << "[LogUtil] " << msg << std::endl;
    // GUI 启动时 stderr 常不可见，额外写 /tmp 便于排查
    try {
        const fs::path fallback = fs::temp_directory_path() / "cpp_project_demo_logutil_error.txt";
        std::ofstream ofs(fallback, std::ios::app);
        if (ofs)
            ofs << msg << '\n';
    } catch (...) {
    }
}

} // namespace

std::string LogUtil::resolveLogDirectory() {
    const fs::path primary = logDirectoryCandidate();
    std::string err;
    if (ensureDirectory(primary, err))
        return primary.string();

    reportFailure("主日志目录不可用: " + err);

    const fs::path fallback = fs::current_path() / "logs";
    if (fallback != primary && ensureDirectory(fallback, err)) {
        reportFailure("已回退到工作目录日志: " + fallback.string());
        return fallback.string();
    }

    const fs::path tmp = fs::temp_directory_path() / "cpp_project_demo_logs";
    if (ensureDirectory(tmp, err)) {
        reportFailure("已回退到临时目录日志: " + tmp.string());
        return tmp.string();
    }

    reportFailure("无法创建任何日志目录，最后错误: " + err);
    return primary.string();
}

std::string LogUtil::defaultLogFilePath(const std::string& log_name) {
    const std::string safe = log_name.empty() ? "app" : log_name;
    return (fs::path(resolveLogDirectory()) / (safe + ".log")).string();
}

void LogUtil::init(const std::string& log_name,
                   const std::string& logfile_path,
                   size_t max_file_size,
                   size_t max_files,
                   bool use_stderr) {
    try {
        std::shared_ptr<spdlog::sinks::sink> console_sink;
        if (use_stderr) {
            console_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        } else {
            console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        }
        console_sink->set_level(spdlog::level::trace);
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

        std::vector<spdlog::sink_ptr> sinks{console_sink};

        std::string resolved_path = logfile_path;
        if (resolved_path.empty()) {
            resolved_path = defaultLogFilePath(log_name);
        } else {
            const fs::path p(resolved_path);
            if (!p.is_absolute()) {
                resolved_path = (fs::path(resolveLogDirectory()) / p.filename()).string();
            } else {
                std::string err;
                if (!p.parent_path().empty() && !ensureDirectory(p.parent_path(), err))
                    reportFailure("无法创建日志父目录: " + err);
            }
        }

        std::string file_error;
        try {
            {
                std::string err;
                const fs::path parent = fs::path(resolved_path).parent_path();
                if (!parent.empty() && !ensureDirectory(parent, err))
                    throw spdlog::spdlog_ex(err);
            }
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                resolved_path, max_file_size, max_files);
            file_sink->set_level(spdlog::level::trace);
            // 业务多用 spdlog::info 而非 SPDLOG_INFO，pattern 用 logger 名而非 %s:%#
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v");
            sinks.push_back(file_sink);
        } catch (const spdlog::spdlog_ex& ex) {
            file_error = std::string("无法创建日志文件 ") + resolved_path + " — " + ex.what();
            reportFailure(file_error + "；降级为仅控制台日志");
        } catch (const std::exception& ex) {
            file_error = std::string("无法创建日志文件 ") + resolved_path + " — " + ex.what();
            reportFailure(file_error + "；降级为仅控制台日志");
        }

        auto logger = std::make_shared<spdlog::logger>(log_name, sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::debug);
        // info 及以上立即落盘，避免短命进程/信号退出时丢缓冲
        logger->flush_on(spdlog::level::info);

        spdlog::set_default_logger(logger);
        spdlog::flush_every(std::chrono::seconds(1));

        if (sinks.size() > 1) {
            spdlog::info("LogUtil 已初始化 — 控制台 + 文件: {}", resolved_path);
        } else {
            spdlog::warn("LogUtil 已初始化 — 仅控制台（文件不可用）: {}",
                         file_error.empty() ? resolved_path : file_error);
        }
        logger->flush();
    } catch (const spdlog::spdlog_ex& ex) {
        reportFailure(std::string("日志初始化失败: ") + ex.what());
        throw;
    }
}

void LogUtil::flush() {
    if (auto logger = spdlog::default_logger())
        logger->flush();
}

void LogUtil::shutdown() {
    spdlog::shutdown();
}
