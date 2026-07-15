#include "core/backup_engine.hpp"

#include <array>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "utils/logger.hpp"
#include "utils/metadata_utils.hpp"
#include "utils/path_utils.hpp"

namespace backup_system::core {

namespace {

std::string describe_path(const std::filesystem::path& path) {
    return path.string();
}

// ---------------------------------------------------------------------------
// ChecksumSinkStream — an ostream that discards all data while computing a
// checksum via the provided engine.  Used by verify mode to validate content
// without writing restored files to disk.
// ---------------------------------------------------------------------------
class ChecksumSinkBuffer final : public std::streambuf {
public:
    explicit ChecksumSinkBuffer(const strategy::IChecksumEngine& engine)
        : engine_(engine),
          state_(engine_.initial_value()) {
    }

    std::uint64_t checksum() const {
        return engine_.finalize(state_);
    }

protected:
    int_type overflow(const int_type value) override {
        if (traits_type::eq_int_type(value, traits_type::eof())) {
            return traits_type::not_eof(value);
        }
        const char byte = traits_type::to_char_type(value);
        engine_.update(state_, &byte, 1);
        return value;
    }

    std::streamsize xsputn(const char* source, std::streamsize count) override {
        if (count > 0) {
            engine_.update(state_, source, static_cast<std::size_t>(count));
        }
        return count;
    }

private:
    const strategy::IChecksumEngine& engine_;
    std::uint64_t state_;
};

class ChecksumSinkStream final : public std::ostream {
public:
    explicit ChecksumSinkStream(const strategy::IChecksumEngine& engine)
        : std::ostream(nullptr),
          buffer_(engine) {
        rdbuf(&buffer_);
    }

    std::uint64_t checksum() const {
        return buffer_.checksum();
    }

private:
    ChecksumSinkBuffer buffer_;
};

}  // namespace

BackupEngine::BackupEngine(std::shared_ptr<strategy::IFileFilter> filter,
                           std::shared_ptr<strategy::IStreamProcessor> stream_processor,
                           std::shared_ptr<strategy::IArchiveStrategy> archive_strategy,
                           std::shared_ptr<strategy::IChecksumEngine> checksum_engine)
    : filter_(std::move(filter)),
      stream_processor_(std::move(stream_processor)),
      archive_strategy_(std::move(archive_strategy)),
      checksum_engine_(std::move(checksum_engine)) {
    if (!filter_) {
        throw std::invalid_argument("file filter must not be null");
    }
    if (!stream_processor_) {
        throw std::invalid_argument("stream processor must not be null");
    }
    if (!archive_strategy_) {
        throw std::invalid_argument("archive strategy must not be null");
    }
    if (!checksum_engine_) {
        throw std::invalid_argument("checksum engine must not be null");
    }
}

void BackupEngine::backup(const BackupOptions& options) const {
    validate_backup_options(options);

    if (options.archive_path.has_parent_path()) {
        std::filesystem::create_directories(options.archive_path.parent_path());
    }
    auto archive_writer = archive_strategy_->create_writer(options.archive_path);
    utils::Logger::info("starting backup from " + describe_path(options.source_root));
    archive_writer->write_directory(".", utils::MetadataUtils::collect(options.source_root));

    for (std::filesystem::recursive_directory_iterator it(options.source_root), end; it != end; ++it) {
        const auto& entry = *it;

        if (!filter_->should_include(entry, options.source_root)) {
            if (entry.is_directory()) {
                it.disable_recursion_pending();
            }
            continue;
        }

        backup_directory_entry(options.source_root, entry, *archive_writer);
    }

    archive_writer->finish();
    utils::Logger::info("backup archive written to " + describe_path(options.archive_path));
}

void BackupEngine::restore(const RestoreOptions& options) const {
    validate_restore_options(options);

    auto archive_reader = archive_strategy_->create_reader(options.archive_path);
    std::filesystem::create_directories(options.restore_root);
    utils::Logger::info("starting restore into " + describe_path(options.restore_root));
    std::vector<DeferredMetadataEntry> deferred_directory_metadata;

    while (true) {
        const auto entry = archive_reader->read_next_entry();
        if (entry.type == strategy::ArchiveEntryType::end_of_archive) {
            break;
        }
        if (entry.type == strategy::ArchiveEntryType::directory) {
            restore_directory(entry, options.restore_root, deferred_directory_metadata);
            continue;
        }
        if (entry.type == strategy::ArchiveEntryType::regular_file) {
            restore_regular_file(entry, *archive_reader, options.restore_root);
            continue;
        }

        throw std::runtime_error("unsupported archive entry type");
    }

    archive_reader->finish();
    apply_deferred_directory_metadata(deferred_directory_metadata);
    utils::Logger::info("restore completed from " + describe_path(options.archive_path));
}

void BackupEngine::validate_backup_options(const BackupOptions& options) const {
    if (options.source_root.empty() || options.archive_path.empty()) {
        throw std::invalid_argument("source and archive paths must not be empty");
    }
    if (!std::filesystem::exists(options.source_root)) {
        throw std::runtime_error("source path does not exist: " + options.source_root.string());
    }
    if (!std::filesystem::is_directory(options.source_root)) {
        throw std::runtime_error("source path is not a directory: " + options.source_root.string());
    }
}

void BackupEngine::validate_restore_options(const RestoreOptions& options) const {
    if (options.archive_path.empty() || options.restore_root.empty()) {
        throw std::invalid_argument("archive and restore paths must not be empty");
    }
    if (!std::filesystem::exists(options.archive_path)) {
        throw std::runtime_error("archive path does not exist: " + options.archive_path.string());
    }
    if (!std::filesystem::is_regular_file(options.archive_path)) {
        throw std::runtime_error("archive path is not a regular file: " + options.archive_path.string());
    }
}

void BackupEngine::backup_directory_entry(const std::filesystem::path& source_root,
                                          const std::filesystem::directory_entry& entry,
                                          strategy::IArchiveWriter& archive_writer) const {
    const auto relative_path =
        utils::PathUtils::normalize_for_storage(std::filesystem::relative(entry.path(), source_root));

    if (entry.is_directory()) {
        archive_writer.write_directory(relative_path, utils::MetadataUtils::collect(entry.path()));
        return;
    }

    if (entry.is_regular_file()) {
        backup_regular_file(entry.path(), relative_path, archive_writer);
    }
}

void BackupEngine::backup_regular_file(const std::filesystem::path& source_path,
                                       const std::filesystem::path& relative_path,
                                       strategy::IArchiveWriter& archive_writer) const {
    std::ifstream input(source_path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to open source file: " + source_path.string());
    }

    const auto original_size = static_cast<std::uint64_t>(std::filesystem::file_size(source_path));
    const auto content_checksum = checksum_engine_->compute_file(source_path);
    auto& archive_output = archive_writer.begin_file(
        relative_path,
        original_size,
        content_checksum,
        utils::MetadataUtils::collect(source_path));
    stream_processor_->backup(input, archive_output, source_path);
    archive_writer.end_file();
}

void BackupEngine::restore_directory(const strategy::ArchiveEntry& entry,
                                     const std::filesystem::path& restore_root,
                                     std::vector<DeferredMetadataEntry>& deferred_metadata) const {
    const auto target_path = entry.relative_path == "."
                                 ? restore_root
                                 : restore_root / entry.relative_path;
    std::filesystem::create_directories(target_path);
    deferred_metadata.push_back({target_path, entry.metadata});
}

void BackupEngine::restore_regular_file(const strategy::ArchiveEntry& entry,
                                        strategy::IArchiveReader& archive_reader,
                                        const std::filesystem::path& restore_root) const {
    const auto relative_path = utils::PathUtils::normalize_for_storage(entry.relative_path);
    const auto target_path = restore_root / relative_path;

    std::filesystem::create_directories(target_path.parent_path());

    std::ofstream output(target_path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("failed to create restore file: " + target_path.string());
    }

    auto& archive_input = archive_reader.current_file_stream();
    stream_processor_->restore(archive_input, output, relative_path);
    archive_reader.finish_file();
    output.flush();
    if (!output) {
        throw std::runtime_error("failed to flush restore file: " + target_path.string());
    }

    const auto restored_size = static_cast<std::uint64_t>(std::filesystem::file_size(target_path));
    if (restored_size != entry.original_size) {
        throw std::runtime_error("restored file size does not match archive metadata for: " + target_path.string());
    }
    if (checksum_engine_->compute_file(target_path) != entry.content_checksum) {
        throw std::runtime_error("restored file checksum does not match archive metadata for: " + target_path.string());
    }

    utils::MetadataUtils::apply(target_path, entry.metadata);
}

void BackupEngine::validate_verify_options(const VerifyOptions& options) const {
    if (options.archive_path.empty()) {
        throw std::invalid_argument("archive path must not be empty");
    }
    if (!std::filesystem::exists(options.archive_path)) {
        throw std::runtime_error("archive path does not exist: " + options.archive_path.string());
    }
    if (!std::filesystem::is_regular_file(options.archive_path)) {
        throw std::runtime_error("archive path is not a regular file: " + options.archive_path.string());
    }
}

void BackupEngine::verify(const VerifyOptions& options) const {
    validate_verify_options(options);

    auto archive_reader = archive_strategy_->create_reader(options.archive_path);
    utils::Logger::info("verifying archive: " + describe_path(options.archive_path));

    // The reader auto-detected the checksum engine from the archive flags.
    // Use it for content verification so we match the algorithm that wrote the archive.
    std::uint64_t passed = 0;
    std::uint64_t failed = 0;
    std::uint64_t total = 0;

    while (true) {
        const auto entry = archive_reader->read_next_entry();
        if (entry.type == strategy::ArchiveEntryType::end_of_archive) {
            break;
        }
        if (entry.type == strategy::ArchiveEntryType::directory) {
            utils::Logger::info("[DIR]  " + entry.relative_path.string());
            continue;
        }
        if (entry.type == strategy::ArchiveEntryType::regular_file) {
            ++total;
            if (verify_regular_file(entry, *archive_reader)) {
                ++passed;
            } else {
                ++failed;
            }
            continue;
        }

        throw std::runtime_error("unsupported archive entry type");
    }

    archive_reader->finish();

    utils::Logger::info("verification summary: " + std::to_string(passed) + " passed, " +
                        std::to_string(failed) + " failed, " + std::to_string(total) + " total");
    if (failed > 0) {
        throw std::runtime_error("archive verification failed: " + std::to_string(failed) +
                                 " file(s) did not pass verification");
    }
}

bool BackupEngine::verify_regular_file(const strategy::ArchiveEntry& entry,
                                        strategy::IArchiveReader& archive_reader) const {
    const auto file_label = entry.relative_path.string();

    try {
        auto& input = archive_reader.current_file_stream();
        ChecksumSinkStream sink(archive_reader.checksum_engine());
        stream_processor_->restore(input, sink, entry.relative_path);
        archive_reader.finish_file();

        if (sink.checksum() != entry.content_checksum) {
            utils::Logger::error("[FAIL] " + file_label + ": content checksum mismatch "
                                 "(expected=" + std::to_string(entry.content_checksum) +
                                 ", actual=" + std::to_string(sink.checksum()) + ")");
            return false;
        }

        utils::Logger::info("[PASS] " + file_label);
        return true;
    } catch (const std::exception& ex) {
        utils::Logger::error("[FAIL] " + file_label + ": " + ex.what());
        archive_reader.skip_current_file();
        return false;
    }
}

void BackupEngine::apply_deferred_directory_metadata(
    const std::vector<DeferredMetadataEntry>& deferred_metadata) const {
    for (auto it = deferred_metadata.rbegin(); it != deferred_metadata.rend(); ++it) {
        utils::MetadataUtils::apply(it->target_path, it->metadata);
    }
}

}  // namespace backup_system::core
