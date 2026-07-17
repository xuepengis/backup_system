#pragma once

#include <string_view>

namespace backup_system::utils {

/// 日志级别枚举。
enum class LogLevel {
    info,
    warning,
    error,
};

/// 轻量日志工具，统一输出到标准错误流。
class Logger {
public:
    /// 输出指定级别的日志。
    static void log(LogLevel level, std::string_view message);

    /// 输出信息级日志。
    static void info(std::string_view message);

    /// 输出警告级日志。
    static void warning(std::string_view message);

    /// 输出错误级日志。
    static void error(std::string_view message);
};

}  // namespace backup_system::utils
