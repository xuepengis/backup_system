#include "utils/logger.hpp"

#include <iostream>

namespace backup_system::utils {

namespace {

// 将内部枚举映射为稳定的日志前缀，便于终端与日志采集统一识别。
std::string_view to_string(const LogLevel level) {
    switch (level) {
    case LogLevel::info:
        return "INFO";
    case LogLevel::warning:
        return "WARN";
    case LogLevel::error:
        return "ERROR";
    }
    return "UNKNOWN";
}

}  // namespace

void Logger::log(const LogLevel level, const std::string_view message) {
    std::cerr << "[" << to_string(level) << "] " << message << '\n';
}

void Logger::info(const std::string_view message) {
    log(LogLevel::info, message);
}

void Logger::warning(const std::string_view message) {
    log(LogLevel::warning, message);
}

void Logger::error(const std::string_view message) {
    log(LogLevel::error, message);
}

}  // namespace backup_system::utils
