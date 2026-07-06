#pragma once

#include <string_view>

namespace backup_system::utils {

enum class LogLevel {
    info,
    warning,
    error,
};

class Logger {
public:
    static void log(LogLevel level, std::string_view message);
    static void info(std::string_view message);
    static void warning(std::string_view message);
    static void error(std::string_view message);
};

}  // namespace backup_system::utils
