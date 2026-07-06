#pragma once

#include <filesystem>
#include <string>

namespace backup_system::utils {

class PathUtils {
public:
    static std::filesystem::path normalize_for_storage(const std::filesystem::path& path);
    static std::string to_generic_string(const std::filesystem::path& path);
    static std::filesystem::path from_generic_string(const std::string& path_text);
};

}  // namespace backup_system::utils
