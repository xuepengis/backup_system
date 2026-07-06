#include "core/backup_engine.hpp"

#include <array>
#include <fstream>
#include <stdexcept>
#include <string>

#include "utils/logger.hpp"
#include "utils/path_utils.hpp"

namespace backup_system::strategy {

bool PassThroughFileFilter::should_include(const std::filesystem::directory_entry& entry) const {
    (void)entry;
    return true;
}

namespace {

void copy_stream(std::istream& input, std::ostream& output) {
    std::array<char, 64 * 1024> buffer {};

    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto bytes_read = input.gcount();
        if (bytes_read > 0) {
            output.write(buffer.data(), bytes_read);
        }
    }

    if (!input.eof()) {
        throw std::runtime_error("stream copy failed while reading input");
    }

    if (!output) {
        throw std::runtime_error("stream copy failed while writing output");
    }
}

}  // namespace

void DirectCopyStreamProcessor::backup(std::istream& input,
                                       std::ostream& output,
                                       const std::filesystem::path& source_path) const {
    (void)source_path;
    copy_stream(input, output);
}

void DirectCopyStreamProcessor::restore(std::istream& input,
                                        std::ostream& output,
                                        const std::filesystem::path& archived_path) const {
    (void)archived_path;
    copy_stream(input, output);
}

}  // namespace backup_system::strategy

namespace backup_system::core {

namespace {
std::string describe_path(const std::filesystem::path& path) {
    return path.string();
}

}  // namespace

BackupEngine::BackupEngine(std::shared_ptr<strategy::IFileFilter> filter,
                           std::shared_ptr<strategy::IStreamProcessor> stream_processor,
                           std::shared_ptr<strategy::IArchiveStrategy> archive_strategy)
    : filter_(std::move(filter)),
      stream_processor_(std::move(stream_processor)),
      archive_strategy_(std::move(archive_strategy)) {
    if (!filter_) {
        throw std::invalid_argument("file filter must not be null");
    }
    if (!stream_processor_) {
        throw std::invalid_argument("stream processor must not be null");
    }
    if (!archive_strategy_) {
        throw std::invalid_argument("archive strategy must not be null");
    }
}

void BackupEngine::backup(const BackupOptions& options) const {
    validate_backup_options(options);

    if (options.archive_path.has_parent_path()) {
        std::filesystem::create_directories(options.archive_path.parent_path());
    }
    auto archive_writer = archive_strategy_->create_writer(options.archive_path);
    utils::Logger::info("starting backup from " + describe_path(options.source_root));

    for (std::filesystem::recursive_directory_iterator it(options.source_root), end; it != end; ++it) {
        const auto& entry = *it;

        if (!filter_->should_include(entry)) {
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

    while (true) {
        const auto entry = archive_reader->read_next_entry();
        if (entry.type == strategy::ArchiveEntryType::end_of_archive) {
            break;
        }
        if (entry.type == strategy::ArchiveEntryType::directory) {
            restore_directory(entry, options.restore_root);
            continue;
        }
        if (entry.type == strategy::ArchiveEntryType::regular_file) {
            restore_regular_file(entry, *archive_reader, options.restore_root);
            continue;
        }

        throw std::runtime_error("unsupported archive entry type");
    }

    archive_reader->finish();
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
        archive_writer.write_directory(relative_path);
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
    auto& archive_output = archive_writer.begin_file(relative_path, original_size);
    stream_processor_->backup(input, archive_output, source_path);
    archive_writer.end_file();
}

void BackupEngine::restore_directory(const strategy::ArchiveEntry& entry,
                                     const std::filesystem::path& restore_root) const {
    const auto target_path = entry.relative_path == "."
                                 ? restore_root
                                 : restore_root / entry.relative_path;
    std::filesystem::create_directories(target_path);
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
}

}  // namespace backup_system::core
