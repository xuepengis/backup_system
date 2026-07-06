#include "utils/metadata_utils.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace backup_system::utils {

namespace {

std::runtime_error make_system_error(const std::string& operation, const std::filesystem::path& path) {
    return std::runtime_error(operation + " failed for " + path.string() + ": " + std::strerror(errno));
}

}  // namespace

FileMetadata MetadataUtils::collect(const std::filesystem::path& path) {
    struct stat stat_buffer {};
    if (::stat(path.c_str(), &stat_buffer) != 0) {
        throw make_system_error("stat", path);
    }

    FileMetadata metadata;
    metadata.mode = static_cast<std::uint32_t>(stat_buffer.st_mode);
    metadata.access_time_sec = static_cast<std::int64_t>(stat_buffer.st_atim.tv_sec);
    metadata.access_time_nsec = static_cast<std::int64_t>(stat_buffer.st_atim.tv_nsec);
    metadata.modification_time_sec = static_cast<std::int64_t>(stat_buffer.st_mtim.tv_sec);
    metadata.modification_time_nsec = static_cast<std::int64_t>(stat_buffer.st_mtim.tv_nsec);
    metadata.owner_uid = static_cast<std::uint32_t>(stat_buffer.st_uid);
    metadata.owner_gid = static_cast<std::uint32_t>(stat_buffer.st_gid);
    return metadata;
}

void MetadataUtils::apply(const std::filesystem::path& path, const FileMetadata& metadata) {
    const mode_t permission_bits = static_cast<mode_t>(metadata.mode & 07777U);
    if (::chmod(path.c_str(), permission_bits) != 0) {
        throw make_system_error("chmod", path);
    }

    if (::chown(path.c_str(),
                static_cast<uid_t>(metadata.owner_uid),
                static_cast<gid_t>(metadata.owner_gid)) != 0) {
        if (errno != EPERM) {
            throw make_system_error("chown", path);
        }
    }

    timespec times[2] {};
    times[0].tv_sec = static_cast<time_t>(metadata.access_time_sec);
    times[0].tv_nsec = static_cast<long>(metadata.access_time_nsec);
    times[1].tv_sec = static_cast<time_t>(metadata.modification_time_sec);
    times[1].tv_nsec = static_cast<long>(metadata.modification_time_nsec);

    if (::utimensat(AT_FDCWD, path.c_str(), times, 0) != 0) {
        throw make_system_error("utimensat", path);
    }
}

}  // namespace backup_system::utils
