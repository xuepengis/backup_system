#pragma once

#include <cstdint>
#include <filesystem>

namespace backup_system::utils {

/// 文件元数据快照，用于归档时保存权限、时间和属主信息。
struct FileMetadata {
    std::uint32_t mode {0};
    std::int64_t access_time_sec {0};
    std::int64_t access_time_nsec {0};
    std::int64_t modification_time_sec {0};
    std::int64_t modification_time_nsec {0};
    std::uint32_t owner_uid {0};
    std::uint32_t owner_gid {0};
};

/// 文件元数据采集与回写工具。
class MetadataUtils {
public:
    /// 采集指定路径的权限、时间戳和属主信息。
    static FileMetadata collect(const std::filesystem::path& path);

    /// 将归档中保存的元数据回写到目标路径。
    static void apply(const std::filesystem::path& path, const FileMetadata& metadata);
};

}  // namespace backup_system::utils
