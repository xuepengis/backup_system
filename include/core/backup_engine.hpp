#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include "strategy/iarchive_strategy.hpp"
#include "strategy/ifile_filter.hpp"
#include "strategy/istream_processor.hpp"
#include "utils/metadata_utils.hpp"

namespace backup_system::core {

struct BackupOptions {
    std::filesystem::path source_root;
    std::filesystem::path archive_path;
};

struct RestoreOptions {
    std::filesystem::path archive_path;
    std::filesystem::path restore_root;
};

class BackupEngine {
public:
    BackupEngine(std::shared_ptr<strategy::IFileFilter> filter,
                 std::shared_ptr<strategy::IStreamProcessor> stream_processor,
                 std::shared_ptr<strategy::IArchiveStrategy> archive_strategy);

    void backup(const BackupOptions& options) const;
    void restore(const RestoreOptions& options) const;

private:
    struct DeferredMetadataEntry {
        std::filesystem::path target_path;
        utils::FileMetadata metadata;
    };

    void validate_backup_options(const BackupOptions& options) const;
    void validate_restore_options(const RestoreOptions& options) const;

    void backup_directory_entry(const std::filesystem::path& source_root,
                                const std::filesystem::directory_entry& entry,
                                strategy::IArchiveWriter& archive_writer) const;

    void backup_regular_file(const std::filesystem::path& source_path,
                             const std::filesystem::path& relative_path,
                             strategy::IArchiveWriter& archive_writer) const;

    void restore_directory(const strategy::ArchiveEntry& entry,
                           const std::filesystem::path& restore_root,
                           std::vector<DeferredMetadataEntry>& deferred_metadata) const;

    void restore_regular_file(const strategy::ArchiveEntry& entry,
                              strategy::IArchiveReader& archive_reader,
                              const std::filesystem::path& restore_root) const;

    void apply_deferred_directory_metadata(const std::vector<DeferredMetadataEntry>& deferred_metadata) const;

    std::shared_ptr<strategy::IFileFilter> filter_;
    std::shared_ptr<strategy::IStreamProcessor> stream_processor_;
    std::shared_ptr<strategy::IArchiveStrategy> archive_strategy_;
};

}  // namespace backup_system::core
