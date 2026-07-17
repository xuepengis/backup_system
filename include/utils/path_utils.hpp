#pragma once

#include <filesystem>
#include <string>

namespace backup_system::utils {

/// 路径工具类，统一处理归档内部路径的规范化与转换。
class PathUtils {
public:
    /// 规范化归档内部路径，并阻止绝对路径与目录逃逸。
    static std::filesystem::path normalize_for_storage(const std::filesystem::path& path);

    /// 将路径转换为跨平台稳定的 generic 字符串。
    static std::string to_generic_string(const std::filesystem::path& path);

    /// 从 generic 字符串恢复并校验归档内部路径。
    static std::filesystem::path from_generic_string(const std::string& path_text);
};

}  // namespace backup_system::utils
