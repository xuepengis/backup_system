#include "utils/path_utils.hpp"

#include <filesystem>
#include <stdexcept>

namespace backup_system::utils {

std::filesystem::path PathUtils::normalize_for_storage(const std::filesystem::path& path) {
    const auto normalized = path.lexically_normal();
    if (normalized.empty()) {
        return {};
    }
    if (normalized.is_absolute()) {
        throw std::invalid_argument("storage path must be relative");
    }
    // 显式拒绝 `..`，避免恢复时写出归档根目录之外。
    for (const auto& component : normalized) {
        if (component == "..") {
            throw std::invalid_argument("storage path must not escape the archive root");
        }
    }
    return normalized;
}

std::string PathUtils::to_generic_string(const std::filesystem::path& path) {
    return normalize_for_storage(path).generic_string();
}

std::filesystem::path PathUtils::from_generic_string(const std::string& path_text) {
    return normalize_for_storage(std::filesystem::path(path_text));
}

}  // namespace backup_system::utils
