#ifndef LOGUTIL_H
#define LOGUTIL_H

#include <spdlog/spdlog.h>

#include <cstddef>
#include <string>

class LogUtil {
public:
    LogUtil() = delete;
    ~LogUtil() = delete;

    /// 解析统一日志目录：优先项目根下 logs/（从 exe/CWD 向上识别 CMakeLists/.git），保证目录存在
    static std::string resolveLogDirectory();

    /// 默认日志文件：{resolveLogDirectory()}/{log_name}.log
    static std::string defaultLogFilePath(const std::string& log_name);

    /// @param log_name      logger 名；logfile_path 为空时也用作默认文件名
    /// @param logfile_path  空=自动路径；相对路径则取文件名拼到 resolveLogDirectory()；绝对路径原样使用
    /// @param use_stderr    true 时控制台走 stderr（子进程 stdout 被协议占用时必须开启）
    static void init(const std::string& log_name = "default_logger",
                     const std::string& logfile_path = "",
                     size_t max_file_size = 1024 * 1024 * 5,  // 5MB
                     size_t max_files = 3,
                     bool use_stderr = false);

    static void flush();
    static void shutdown();
};

#endif // LOGUTIL_H
