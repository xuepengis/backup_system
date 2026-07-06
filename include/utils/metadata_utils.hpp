#pragma once

#include <cstdint>
#include <filesystem>

namespace backup_system::utils {

struct FileMetadata {
    std::uint32_t mode {0};
    std::int64_t access_time_sec {0};
    std::int64_t access_time_nsec {0};
    std::int64_t modification_time_sec {0};
    std::int64_t modification_time_nsec {0};
    std::uint32_t owner_uid {0};
    std::uint32_t owner_gid {0};
};

class MetadataUtils {
public:
    static FileMetadata collect(const std::filesystem::path& path);
    static void apply(const std::filesystem::path& path, const FileMetadata& metadata);
};

}  // namespace backup_system::utils
