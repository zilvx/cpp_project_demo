#ifndef LOGUTIL_H
#define LOGUTIL_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <vector>
#include <string>
#include <iostream>

class LogUtil {
public:

    LogUtil() = delete; // Prevent instantiation    
    ~LogUtil() = delete; // Prevent destruction

    static void init(const std::string& log_name = "default_logger",
                     const std::string& logfile_path = "logs/app.log",
                     size_t max_file_size = 1024 * 1024 * 5,  // 5MB
                     size_t max_files = 3);

    static void flush();

    static void shutdown();
};

#endif // LOGUTIL_H