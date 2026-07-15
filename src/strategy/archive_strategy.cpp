#include "strategy/iarchive_strategy.hpp"

#include <array>
#include <cstddef>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

#include <sys/stat.h>

#include "strategy/ichecksum_engine.hpp"
#include "strategy/codec_registry.hpp"
#include "utils/path_utils.hpp"

namespace backup_system::strategy {

namespace {

constexpr std::array<char, 4> kArchiveMagic {'B', 'K', 'S', '1'};
constexpr std::uint16_t kArchiveVersion = 3;

template <typename T>
void write_binary(std::ostream& output, const T& value) {
    output.write(reinterpret_cast<const char*>(&value), static_cast<std::streamsize>(sizeof(T)));
    if (!output) {
        throw std::runtime_error("failed to write archive data");
    }
}

template <typename T>
T read_binary(std::istream& input) {
    T value {};
    input.read(reinterpret_cast<char*>(&value), static_cast<std::streamsize>(sizeof(T)));
    if (!input) {
        throw std::runtime_error("failed to read archive data");
    }
    return value;
}

std::uint64_t checksum_path(const std::string& path_text, const IChecksumEngine& engine) {
    auto state = engine.initial_value();
    engine.update(state, path_text.data(), path_text.size());
    return engine.finalize(state);
}

void write_string(std::ostream& output, const std::string& value) {
    write_binary(output, static_cast<std::uint16_t>(value.size()));
    output.write(value.data(), static_cast<std::streamsize>(value.size()));
    if (!output) {
        throw std::runtime_error("failed to write archive string");
    }
}

std::string read_string(std::istream& input) {
    const auto size = read_binary<std::uint16_t>(input);
    std::string value(size, '\0');
    if (size > 0) {
        input.read(value.data(), static_cast<std::streamsize>(size));
        if (!input) {
            throw std::runtime_error("failed to read archive string");
        }
    }
    return value;
}

class LimitedStreamBuffer : public std::streambuf {
public:
    LimitedStreamBuffer(std::streambuf* source, std::uint64_t remaining_bytes)
        : source_(source),
          remaining_bytes_(remaining_bytes) {
    }

    std::uint64_t remaining_bytes() const {
        return remaining_bytes_;
    }

protected:
    int_type underflow() override {
        if (remaining_bytes_ == 0) {
            return traits_type::eof();
        }

        const auto next = source_->sgetc();
        if (traits_type::eq_int_type(next, traits_type::eof())) {
            return traits_type::eof();
        }

        buffer_[0] = traits_type::to_char_type(next);
        setg(buffer_.data(), buffer_.data(), buffer_.data() + 1);
        return traits_type::to_int_type(buffer_[0]);
    }

    int_type uflow() override {
        const auto next = underflow();
        if (traits_type::eq_int_type(next, traits_type::eof())) {
            return next;
        }

        source_->sbumpc();
        --remaining_bytes_;
        setg(buffer_.data(), buffer_.data() + 1, buffer_.data() + 1);
        return next;
    }

    std::streamsize xsgetn(char* destination, std::streamsize count) override {
        if (remaining_bytes_ == 0 || count <= 0) {
            return 0;
        }

        const auto bounded_count = static_cast<std::streamsize>(
            std::min<std::uint64_t>(remaining_bytes_, static_cast<std::uint64_t>(count)));
        const auto read_count = source_->sgetn(destination, bounded_count);
        remaining_bytes_ -= static_cast<std::uint64_t>(read_count);
        return read_count;
    }

private:
    std::streambuf* source_;
    std::uint64_t remaining_bytes_;
    std::array<char, 1> buffer_ {};
};

class ChecksumLimitedStreamBuffer : public LimitedStreamBuffer {
public:
    ChecksumLimitedStreamBuffer(std::streambuf* source,
                                 const std::uint64_t remaining_bytes,
                                 const IChecksumEngine& engine)
        : LimitedStreamBuffer(source, remaining_bytes),
          engine_(engine),
          state_(engine_.initial_value()) {
    }

    std::streamsize xsgetn(char* destination, std::streamsize count) override {
        const auto read_count = LimitedStreamBuffer::xsgetn(destination, count);
        if (read_count > 0) {
            engine_.update(state_, destination, static_cast<std::size_t>(read_count));
        }
        return read_count;
    }

    int_type uflow() override {
        const auto value = LimitedStreamBuffer::uflow();
        if (!traits_type::eq_int_type(value, traits_type::eof())) {
            const char byte = traits_type::to_char_type(value);
            engine_.update(state_, &byte, 1);
        }
        return value;
    }

    std::uint64_t hash() const {
        return engine_.finalize(state_);
    }

private:
    const IChecksumEngine& engine_;
    std::uint64_t state_;
};

class ChecksumLimitedInputStream final : public std::istream {
public:
    ChecksumLimitedInputStream(std::istream& source,
                                const std::uint64_t remaining_bytes,
                                const IChecksumEngine& engine)
        : std::istream(nullptr),
          buffer_(source.rdbuf(), remaining_bytes, engine) {
        rdbuf(&buffer_);
    }

    std::uint64_t remaining_bytes() const {
        return buffer_.remaining_bytes();
    }

    std::uint64_t hash() const {
        return buffer_.hash();
    }

private:
    ChecksumLimitedStreamBuffer buffer_;
};

class ChecksumOutputStreamBuffer final : public std::streambuf {
public:
    ChecksumOutputStreamBuffer(std::streambuf* destination, const IChecksumEngine& engine)
        : destination_(destination),
          engine_(engine),
          state_(engine_.initial_value()) {
    }

    std::uint64_t bytes_written() const {
        return bytes_written_;
    }

    std::uint64_t hash() const {
        return engine_.finalize(state_);
    }

protected:
    int_type overflow(const int_type value) override {
        if (traits_type::eq_int_type(value, traits_type::eof())) {
            return traits_type::not_eof(value);
        }

        const char byte = traits_type::to_char_type(value);
        if (traits_type::eq_int_type(destination_->sputc(byte), traits_type::eof())) {
            return traits_type::eof();
        }

        engine_.update(state_, &byte, 1);
        ++bytes_written_;
        return value;
    }

    std::streamsize xsputn(const char* source, std::streamsize count) override {
        if (count <= 0) {
            return 0;
        }

        const auto written = destination_->sputn(source, count);
        if (written > 0) {
            engine_.update(state_, source, static_cast<std::size_t>(written));
            bytes_written_ += static_cast<std::uint64_t>(written);
        }
        return written;
    }

    int sync() override {
        return destination_->pubsync();
    }

private:
    std::streambuf* destination_;
    const IChecksumEngine& engine_;
    std::uint64_t bytes_written_ {0};
    std::uint64_t state_;
};

class ChecksumOutputStream final : public std::ostream {
public:
    ChecksumOutputStream(std::ostream& destination, const IChecksumEngine& engine)
        : std::ostream(nullptr),
          buffer_(destination.rdbuf(), engine) {
        rdbuf(&buffer_);
    }

    std::uint64_t bytes_written() const {
        return buffer_.bytes_written();
    }

    std::uint64_t hash() const {
        return buffer_.hash();
    }

private:
    ChecksumOutputStreamBuffer buffer_;
};

void write_archive_header(std::ostream& archive,
                          const PayloadCodecDescriptor& descriptor,
                          const std::uint16_t flags) {
    archive.write(kArchiveMagic.data(), static_cast<std::streamsize>(kArchiveMagic.size()));
    if (!archive) {
        throw std::runtime_error("failed to write archive header");
    }
    write_binary(archive, kArchiveVersion);
    write_binary(archive, flags);
    write_string(archive, descriptor.compression_name);
    write_string(archive, descriptor.encryption_name);
}

struct VerifiedHeader {
    PayloadCodecDescriptor descriptor;
    std::uint16_t flags {0};
};

VerifiedHeader verify_archive_header(std::istream& archive) {
    std::array<char, 4> magic {};
    archive.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    if (!archive) {
        throw std::runtime_error("failed to read archive header");
    }
    if (magic != kArchiveMagic) {
        throw std::runtime_error("invalid archive format");
    }

    const auto version = read_binary<std::uint16_t>(archive);
    const auto flags = read_binary<std::uint16_t>(archive);
    if (version != kArchiveVersion) {
        throw std::runtime_error("unsupported archive version: " + std::to_string(version));
    }
    if ((flags & ~kChecksumFlagMask) != 0) {
        throw std::runtime_error("unsupported archive flags: " + std::to_string(flags));
    }
    return {
        .descriptor = {
            .compression_name = read_string(archive),
            .encryption_name = read_string(archive),
        },
        .flags = flags,
    };
}

void write_archive_entry_header(std::ostream& archive, const ArchiveEntry& entry) {
    write_binary(archive, static_cast<std::uint8_t>(entry.type));

    const auto path_text = entry.type == ArchiveEntryType::end_of_archive
                               ? std::string {}
                               : utils::PathUtils::to_generic_string(entry.relative_path);
    const auto path_length = static_cast<std::uint64_t>(path_text.size());
    write_binary(archive, path_length);
    if (path_length > 0) {
        archive.write(path_text.data(), static_cast<std::streamsize>(path_text.size()));
        if (!archive) {
            throw std::runtime_error("failed to write archive path data");
        }
    }

    write_binary(archive, entry.stored_size);
    write_binary(archive, entry.original_size);
    write_binary(archive, entry.checksum);
    write_binary(archive, entry.content_checksum);
    write_binary(archive, entry.metadata.mode);
    write_binary(archive, entry.metadata.access_time_sec);
    write_binary(archive, entry.metadata.access_time_nsec);
    write_binary(archive, entry.metadata.modification_time_sec);
    write_binary(archive, entry.metadata.modification_time_nsec);
    write_binary(archive, entry.metadata.owner_uid);
    write_binary(archive, entry.metadata.owner_gid);
}

ArchiveEntry read_archive_entry_header(std::istream& archive) {
    const auto raw_type = read_binary<std::uint8_t>(archive);
    const auto path_length = read_binary<std::uint64_t>(archive);
    if (path_length > static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max())) {
        throw std::runtime_error("archive path is too large");
    }

    std::string path_text(path_length, '\0');
    if (path_length > 0) {
        archive.read(path_text.data(), static_cast<std::streamsize>(path_length));
        if (!archive) {
            throw std::runtime_error("failed to read archive path data");
        }
    }

    ArchiveEntry entry;
    entry.type = static_cast<ArchiveEntryType>(raw_type);
    entry.relative_path = path_text.empty() ? std::filesystem::path {} : utils::PathUtils::from_generic_string(path_text);
    entry.stored_size = read_binary<std::uint64_t>(archive);
    entry.original_size = read_binary<std::uint64_t>(archive);
    entry.checksum = read_binary<std::uint64_t>(archive);
    entry.content_checksum = read_binary<std::uint64_t>(archive);
    entry.metadata.mode = read_binary<std::uint32_t>(archive);
    entry.metadata.access_time_sec = read_binary<std::int64_t>(archive);
    entry.metadata.access_time_nsec = read_binary<std::int64_t>(archive);
    entry.metadata.modification_time_sec = read_binary<std::int64_t>(archive);
    entry.metadata.modification_time_nsec = read_binary<std::int64_t>(archive);
    entry.metadata.owner_uid = read_binary<std::uint32_t>(archive);
    entry.metadata.owner_gid = read_binary<std::uint32_t>(archive);
    return entry;
}

void validate_archive_entry(const ArchiveEntry& entry,
                            const bool is_first_entry,
                            const IChecksumEngine& engine) {
    switch (entry.type) {
    case ArchiveEntryType::directory:
        if (entry.relative_path.empty()) {
            throw std::runtime_error("directory entry must contain a path");
        }
        if (entry.stored_size != 0 || entry.original_size != 0) {
            throw std::runtime_error("directory entry must not contain payload sizes");
        }
        if (entry.content_checksum != 0) {
            throw std::runtime_error("directory entry must not contain content checksum");
        }
        if (checksum_path(utils::PathUtils::to_generic_string(entry.relative_path), engine) != entry.checksum) {
            throw std::runtime_error("directory entry checksum mismatch");
        }
        if ((entry.metadata.mode & S_IFMT) != S_IFDIR) {
            throw std::runtime_error("directory entry metadata mode mismatch");
        }
        if (is_first_entry && entry.relative_path != ".") {
            throw std::runtime_error("first archive entry must be the root directory marker");
        }
        return;
    case ArchiveEntryType::regular_file:
        if (entry.relative_path.empty() || entry.relative_path == ".") {
            throw std::runtime_error("regular file entry must contain a non-root path");
        }
        if (entry.checksum == 0 && entry.stored_size != 0) {
            throw std::runtime_error("regular file entry with payload must contain a checksum");
        }
        if (entry.content_checksum == 0 && entry.original_size != 0) {
            throw std::runtime_error("regular file entry with content must contain a content checksum");
        }
        if ((entry.metadata.mode & S_IFMT) != S_IFREG) {
            throw std::runtime_error("regular file entry metadata mode mismatch");
        }
        return;
    case ArchiveEntryType::end_of_archive:
        if (!entry.relative_path.empty()) {
            throw std::runtime_error("end-of-archive entry must not contain a path");
        }
        if (entry.stored_size != 0 || entry.original_size != 0 || entry.checksum != 0 || entry.content_checksum != 0) {
            throw std::runtime_error("end-of-archive entry must have zeroed metadata");
        }
        if (entry.metadata.mode != 0 ||
            entry.metadata.access_time_sec != 0 ||
            entry.metadata.access_time_nsec != 0 ||
            entry.metadata.modification_time_sec != 0 ||
            entry.metadata.modification_time_nsec != 0 ||
            entry.metadata.owner_uid != 0 ||
            entry.metadata.owner_gid != 0) {
            throw std::runtime_error("end-of-archive entry must have zeroed metadata fields");
        }
        if (is_first_entry) {
            throw std::runtime_error("archive cannot start with end-of-archive entry");
        }
        return;
    }

    throw std::runtime_error("unknown archive entry type");
}

class BinaryArchiveWriter final : public IArchiveWriter {
public:
    BinaryArchiveWriter(const std::filesystem::path& archive_path,
                        PayloadCodecDescriptor descriptor,
                        const IChecksumEngine& checksum_engine)
        : archive_(archive_path, std::ios::binary | std::ios::trunc),
          descriptor_(std::move(descriptor)),
          checksum_engine_(checksum_engine) {
        if (!archive_) {
            throw std::runtime_error("failed to open archive for writing");
        }
        write_archive_header(archive_, descriptor_,
                             checksum_flags_from_name(checksum_engine_.name()));
    }

    void write_directory(const std::filesystem::path& relative_path,
                         const utils::FileMetadata& metadata) override {
        const auto normalized_path = utils::PathUtils::normalize_for_storage(relative_path);
        const auto path_text = utils::PathUtils::to_generic_string(normalized_path);
        write_archive_entry_header(
            archive_,
            ArchiveEntry {ArchiveEntryType::directory, normalized_path, 0, 0,
                          checksum_path(path_text, checksum_engine_), 0, metadata});
        if (normalized_path == ".") {
            root_written_ = true;
        }
    }

    std::ostream& begin_file(const std::filesystem::path& relative_path,
                             const std::uint64_t original_size,
                             const std::uint64_t content_checksum,
                             const utils::FileMetadata& metadata) override {
        if (file_open_) {
            throw std::runtime_error("archive file payload is already open");
        }

        current_relative_path_ = utils::PathUtils::normalize_for_storage(relative_path);
        current_original_size_ = original_size;
        current_content_checksum_ = content_checksum;
        current_metadata_ = metadata;
        current_header_position_ = archive_.tellp();
        if (current_header_position_ == std::streampos(-1)) {
            throw std::runtime_error("failed to capture archive header position");
        }

        write_archive_entry_header(
            archive_,
            ArchiveEntry {
                ArchiveEntryType::regular_file,
                current_relative_path_,
                0,
                current_original_size_,
                0,
                current_content_checksum_,
                current_metadata_,
            });

        payload_stream_ = std::make_unique<ChecksumOutputStream>(archive_, checksum_engine_);
        file_open_ = true;
        return *payload_stream_;
    }

    void end_file() override {
        if (!file_open_ || !payload_stream_) {
            throw std::runtime_error("archive file payload is not open");
        }

        payload_stream_->flush();
        if (!*payload_stream_) {
            throw std::runtime_error("failed while writing archived payload");
        }

        const auto end_position = archive_.tellp();
        if (end_position == std::streampos(-1)) {
            throw std::runtime_error("failed to capture archive end position");
        }

        const ArchiveEntry updated_entry {
            ArchiveEntryType::regular_file,
            current_relative_path_,
            payload_stream_->bytes_written(),
            current_original_size_,
            payload_stream_->hash(),
            current_content_checksum_,
            current_metadata_,
        };

        archive_.seekp(current_header_position_);
        if (!archive_) {
            throw std::runtime_error("failed to rewind archive header for update");
        }
        write_archive_entry_header(archive_, updated_entry);
        archive_.seekp(end_position);
        if (!archive_) {
            throw std::runtime_error("failed to restore archive write position");
        }

        payload_stream_.reset();
        file_open_ = false;
    }

    void finish() override {
        if (file_open_) {
            throw std::runtime_error("cannot finish archive while file payload is open");
        }
        if (!root_written_) {
            throw std::runtime_error("archive root entry was not written");
        }
        write_archive_entry_header(archive_, ArchiveEntry {ArchiveEntryType::end_of_archive, {}, 0, 0, 0, 0, {}});
        archive_.flush();
        if (!archive_) {
            throw std::runtime_error("failed to finalize archive");
        }
    }

private:
    std::ofstream archive_;
    bool root_written_ {false};
    bool file_open_ {false};
    std::filesystem::path current_relative_path_;
    std::uint64_t current_original_size_ {0};
    std::uint64_t current_content_checksum_ {0};
    utils::FileMetadata current_metadata_ {};
    std::streampos current_header_position_ {};
    std::unique_ptr<ChecksumOutputStream> payload_stream_;
    PayloadCodecDescriptor descriptor_;
    const IChecksumEngine& checksum_engine_;
};

class BinaryArchiveReader final : public IArchiveReader {
public:
    BinaryArchiveReader(const std::filesystem::path& archive_path, PayloadCodecDescriptor descriptor)
        : archive_(archive_path, std::ios::binary),
          expected_descriptor_(std::move(descriptor)) {
        if (!archive_) {
            throw std::runtime_error("failed to open archive for reading");
        }

        const auto verified = verify_archive_header(archive_);
        if (verified.descriptor.compression_name != expected_descriptor_.compression_name) {
            throw std::runtime_error(
                "archive compression codec mismatch: archive=" + verified.descriptor.compression_name +
                ", expected=" + expected_descriptor_.compression_name);
        }
        if (verified.descriptor.encryption_name != expected_descriptor_.encryption_name) {
            throw std::runtime_error(
                "archive encryption codec mismatch: archive=" + verified.descriptor.encryption_name +
                ", expected=" + expected_descriptor_.encryption_name);
        }

        checksum_engine_ = create_checksum_engine(checksum_name_from_flags(verified.flags));
    }

    const IChecksumEngine& checksum_engine() const override {
        return *checksum_engine_;
    }

    ArchiveEntry read_next_entry() override {
        if (pending_file_) {
            throw std::runtime_error("current archive file entry must be fully consumed before reading next entry");
        }

        current_entry_ = read_archive_entry_header(archive_);
        validate_archive_entry(current_entry_, is_first_entry_, *checksum_engine_);
        is_first_entry_ = false;
        saw_end_of_archive_ = current_entry_.type == ArchiveEntryType::end_of_archive;

        if (current_entry_.type == ArchiveEntryType::regular_file) {
            payload_stream_ = std::make_unique<ChecksumLimitedInputStream>(
                archive_, current_entry_.stored_size, *checksum_engine_);
            pending_file_ = true;
        }

        return current_entry_;
    }

    std::istream& current_file_stream() override {
        if (!pending_file_ || !payload_stream_) {
            throw std::runtime_error("no current archive file payload is open");
        }
        return *payload_stream_;
    }

    void finish_file() override {
        if (!pending_file_ || !payload_stream_) {
            throw std::runtime_error("no current archive file payload is open");
        }
        if (payload_stream_->remaining_bytes() != 0) {
            throw std::runtime_error("archive file payload was not fully consumed");
        }
        if (payload_stream_->hash() != current_entry_.checksum) {
            throw std::runtime_error("regular file payload checksum mismatch");
        }
        payload_stream_.reset();
        pending_file_ = false;
    }

    void skip_current_file() override {
        if (!pending_file_ || !payload_stream_) {
            return;
        }
        // Discard remaining payload bytes without validating checksum.
        std::array<char, 64 * 1024> discard_buffer {};
        while (payload_stream_->remaining_bytes() > 0) {
            payload_stream_->read(
                discard_buffer.data(),
                static_cast<std::streamsize>(
                    std::min(static_cast<std::uint64_t>(discard_buffer.size()),
                             payload_stream_->remaining_bytes())));
        }
        payload_stream_.reset();
        pending_file_ = false;
    }

    void finish() override {
        if (pending_file_) {
            throw std::runtime_error("cannot finish archive reader while file payload is open");
        }
        if (!saw_end_of_archive_) {
            throw std::runtime_error("archive terminated without end marker");
        }
        if (archive_.peek() != std::char_traits<char>::eof()) {
            throw std::runtime_error("unexpected trailing bytes after end-of-archive marker");
        }
    }

private:
    std::ifstream archive_;
    bool is_first_entry_ {true};
    bool saw_end_of_archive_ {false};
    bool pending_file_ {false};
    ArchiveEntry current_entry_ {};
    std::unique_ptr<ChecksumLimitedInputStream> payload_stream_;
    PayloadCodecDescriptor expected_descriptor_;
    std::shared_ptr<IChecksumEngine> checksum_engine_;
};

}  // namespace

BinaryArchiveStrategy::BinaryArchiveStrategy(PayloadCodecDescriptor descriptor,
                                           std::shared_ptr<IChecksumEngine> checksum_engine)
    : descriptor_(std::move(descriptor)),
      checksum_engine_(std::move(checksum_engine)) {
    if (!checksum_engine_) {
        throw std::invalid_argument("checksum engine must not be null");
    }
}

std::unique_ptr<IArchiveWriter> BinaryArchiveStrategy::create_writer(const std::filesystem::path& archive_path) const {
    return std::make_unique<BinaryArchiveWriter>(archive_path, descriptor_, *checksum_engine_);
}

std::unique_ptr<IArchiveReader> BinaryArchiveStrategy::create_reader(const std::filesystem::path& archive_path) const {
    return std::make_unique<BinaryArchiveReader>(archive_path, descriptor_);
}

std::string BinaryArchiveStrategy::name() const {
    return "binary-archive-v3";
}

}  // namespace backup_system::strategy
