#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include "strategy/iarchive_strategy.hpp"
#include "strategy/ichecksum_engine.hpp"
#include "strategy/ifile_filter.hpp"
#include "strategy/istream_processor.hpp"
#include "utils/metadata_utils.hpp"

namespace backup_system::core {

/// 备份流程输入参数。
struct BackupOptions {
    std::filesystem::path source_root;
    std::filesystem::path archive_path;
};

/// 恢复流程输入参数。
struct RestoreOptions {
    std::filesystem::path archive_path;
    std::filesystem::path restore_root;
};

/// 归档校验流程输入参数。
struct VerifyOptions {
    std::filesystem::path archive_path;
};

/// 备份系统核心编排器，负责串联过滤、编解码、归档与校验能力。
class BackupEngine {
public:
    /// 通过策略对象组装完整的备份执行管线。
    BackupEngine(std::shared_ptr<strategy::IFileFilter> filter,
                 std::shared_ptr<strategy::IStreamProcessor> stream_processor,
                 std::shared_ptr<strategy::IArchiveStrategy> archive_strategy,
                 std::shared_ptr<strategy::IChecksumEngine> checksum_engine);

    /// 执行目录备份并写入归档文件。
    void backup(const BackupOptions& options) const;

    /// 将归档文件恢复到目标目录。
    void restore(const RestoreOptions& options) const;

    /// 校验归档内容的可读性与文件校验和。
    void verify(const VerifyOptions& options) const;

private:
    /// 目录元数据需要在子项恢复完成后再回写，避免被创建过程覆盖。
    struct DeferredMetadataEntry {
        std::filesystem::path target_path;
        utils::FileMetadata metadata;
    };

    void validate_backup_options(const BackupOptions& options) const;
    void validate_restore_options(const RestoreOptions& options) const;
    void validate_verify_options(const VerifyOptions& options) const;

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

    bool verify_regular_file(const strategy::ArchiveEntry& entry,
                             strategy::IArchiveReader& archive_reader) const;

    void apply_deferred_directory_metadata(const std::vector<DeferredMetadataEntry>& deferred_metadata) const;

    /// 文件过滤策略。
    std::shared_ptr<strategy::IFileFilter> filter_;
    /// 负责压缩、加密等流式处理。
    std::shared_ptr<strategy::IStreamProcessor> stream_processor_;
    /// 归档格式读写策略。
    std::shared_ptr<strategy::IArchiveStrategy> archive_strategy_;
    /// 内容校验算法。
    std::shared_ptr<strategy::IChecksumEngine> checksum_engine_;
};

}  // namespace backup_system::core
