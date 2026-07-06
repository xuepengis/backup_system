#pragma once

#include <cstdint>
#include <filesystem>
#include <istream>
#include <memory>
#include <ostream>
#include <string>

#include "strategy/istream_processor.hpp"
#include "utils/metadata_utils.hpp"

namespace backup_system::strategy {

enum class ArchiveEntryType : std::uint8_t {
    directory = 1,
    regular_file = 2,
    end_of_archive = 255,
};

struct ArchiveEntry {
    ArchiveEntryType type {ArchiveEntryType::directory};
    std::filesystem::path relative_path;
    std::uint64_t stored_size {0};
    std::uint64_t original_size {0};
    std::uint64_t checksum {0};
    std::uint64_t content_checksum {0};
    utils::FileMetadata metadata {};
};

class IArchiveWriter {
public:
    virtual ~IArchiveWriter() = default;

    virtual void write_directory(const std::filesystem::path& relative_path,
                                 const utils::FileMetadata& metadata) = 0;
    virtual std::ostream& begin_file(const std::filesystem::path& relative_path,
                                     std::uint64_t original_size,
                                     std::uint64_t content_checksum,
                                     const utils::FileMetadata& metadata) = 0;
    virtual void end_file() = 0;
    virtual void finish() = 0;
};

class IArchiveReader {
public:
    virtual ~IArchiveReader() = default;

    virtual ArchiveEntry read_next_entry() = 0;
    virtual std::istream& current_file_stream() = 0;
    virtual void finish_file() = 0;
    virtual void finish() = 0;
};

class IArchiveStrategy {
public:
    virtual ~IArchiveStrategy() = default;

    virtual std::unique_ptr<IArchiveWriter> create_writer(const std::filesystem::path& archive_path) const = 0;
    virtual std::unique_ptr<IArchiveReader> create_reader(const std::filesystem::path& archive_path) const = 0;
    virtual std::string name() const = 0;
};

class BinaryArchiveStrategy final : public IArchiveStrategy {
public:
    explicit BinaryArchiveStrategy(PayloadCodecDescriptor descriptor);

    std::unique_ptr<IArchiveWriter> create_writer(const std::filesystem::path& archive_path) const override;
    std::unique_ptr<IArchiveReader> create_reader(const std::filesystem::path& archive_path) const override;
    std::string name() const override;

private:
    PayloadCodecDescriptor descriptor_;
};

}  // namespace backup_system::strategy
