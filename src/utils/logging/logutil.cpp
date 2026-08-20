#include "logutil.h"

void LogUtil::init(const std::string& log_name,
                   const std::string& logfile_path,
                   size_t max_file_size,
                   size_t max_files) {
    try {
        // 创建控制台sink（带颜色）
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

        // 创建组合sink的logger
        std::vector<spdlog::sink_ptr> sinks{console_sink};

        // 创建文件sink（自动切割）—— 失败时优雅降级为纯控制台日志
        try {
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                logfile_path, max_file_size, max_files
            );
            file_sink->set_level(spdlog::level::trace);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] %v");
            sinks.push_back(file_sink);
        } catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Warning: Could not create log file at " << logfile_path
                      << " — " << ex.what() << ". Falling back to console-only logging."
                      << std::endl;
        }

        auto logger = std::make_shared<spdlog::logger>(log_name, sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::debug);

        // 注册为默认logger
        spdlog::set_default_logger(logger);
        spdlog::flush_every(std::chrono::seconds(1));  // 每1秒自动flush

        if (sinks.size() > 1) {
            spdlog::info("LogUtil initialized - console and file logging active");
        } else {
            spdlog::warn("LogUtil initialized - console-only logging (file logging unavailable)");
        }
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        throw;
    }
}  

void LogUtil::flush() {
    spdlog::default_logger()->flush();
}

void LogUtil::shutdown() {
    spdlog::shutdown();
}