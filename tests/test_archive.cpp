#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <sys/stat.h>

#include "strategy/iarchive_strategy.hpp"

namespace fs = std::filesystem;

using backup_system::strategy::ArchiveEntryType;
using backup_system::strategy::BinaryArchiveStrategy;
using backup_system::strategy::PayloadCodecDescriptor;
using backup_system::utils::FileMetadata;

namespace {

PayloadCodecDescriptor default_descriptor() {
    return PayloadCodecDescriptor{.compression_name = "none",
                                  .encryption_name = "none"};
}

FileMetadata basic_file_metadata() {
    return FileMetadata{.mode = S_IFREG | 0644,
                        .access_time_sec = 1000000,
                        .modification_time_sec = 2000000,
                        .owner_uid = 1000,
                        .owner_gid = 1000};
}

FileMetadata basic_dir_metadata() {
    return FileMetadata{.mode = S_IFDIR | 0755,
                        .access_time_sec = 1000000,
                        .modification_time_sec = 2000000,
                        .owner_uid = 1000,
                        .owner_gid = 1000};
}

struct ArchiveTestFixture {
    fs::path archive_path;
    BinaryArchiveStrategy strategy;

    ArchiveTestFixture()
        : archive_path(fs::temp_directory_path() / "test_archive.bks"),
          strategy(default_descriptor()) {}

    ~ArchiveTestFixture() {
        std::error_code ec;
        fs::remove(archive_path, ec);
    }
};

}  // namespace

// ===========================================================================
// Archive header validation
// ===========================================================================

TEST_CASE("Write and read archive header", "[archive]") {
    ArchiveTestFixture fix;

    // Write a minimal valid archive: root entry + end-of-archive
    {
        auto writer = fix.strategy.create_writer(fix.archive_path);
        writer->write_directory(".", basic_dir_metadata());
        writer->finish();
    }

    // Verify the magic bytes directly in the file
    std::ifstream file(fix.archive_path, std::ios::binary);
    REQUIRE(file.is_open());

    char magic[4] = {};
    file.read(magic, 4);
    REQUIRE(file.good());
    REQUIRE(magic[0] == 'B');
    REQUIRE(magic[1] == 'K');
    REQUIRE(magic[2] == 'S');
    REQUIRE(magic[3] == '1');

    // Read back through reader — header should be accepted
    auto reader = fix.strategy.create_reader(fix.archive_path);
    REQUIRE(reader != nullptr);
}

// ===========================================================================
// Archive entry validation
// ===========================================================================

TEST_CASE("Archive entry types directory/file/EOA validated", "[archive]") {
    ArchiveTestFixture fix;

    // Write archive: root dir + one file + EOA
    {
        auto writer = fix.strategy.create_writer(fix.archive_path);
        writer->write_directory(".", basic_dir_metadata());

        auto& file_stream =
            writer->begin_file("test.txt", 12, 0xDEADBEEF, basic_file_metadata());
        file_stream.write("Hello World!", 12);
        writer->end_file();

        writer->finish();
    }

    // Read back
    auto reader = fix.strategy.create_reader(fix.archive_path);

    // First entry: root directory
    auto root_entry = reader->read_next_entry();
    REQUIRE(root_entry.type == ArchiveEntryType::directory);
    REQUIRE(root_entry.relative_path == ".");
    REQUIRE(root_entry.stored_size == 0);
    REQUIRE(root_entry.original_size == 0);

    // Second entry: regular file
    auto file_entry = reader->read_next_entry();
    REQUIRE(file_entry.type == ArchiveEntryType::regular_file);
    REQUIRE(file_entry.relative_path == "test.txt");
    REQUIRE(file_entry.stored_size == 12);
    REQUIRE(file_entry.original_size == 12);
    REQUIRE_FALSE(file_entry.relative_path.empty());

    // Consume payload and finish file
    auto& payload = reader->current_file_stream();
    std::string content(static_cast<std::size_t>(file_entry.stored_size), '\0');
    payload.read(content.data(), static_cast<std::streamsize>(file_entry.stored_size));
    reader->finish_file();

    // Third entry: end-of-archive
    auto eoa_entry = reader->read_next_entry();
    REQUIRE(eoa_entry.type == ArchiveEntryType::end_of_archive);
    REQUIRE(eoa_entry.relative_path.empty());
    REQUIRE(eoa_entry.stored_size == 0);
    REQUIRE(eoa_entry.original_size == 0);
    REQUIRE(eoa_entry.checksum == 0);
    REQUIRE(eoa_entry.content_checksum == 0);

    reader->finish();
}

// ===========================================================================
// Full write+read cycle
// ===========================================================================

TEST_CASE("Binary archive full write+read cycle", "[archive]") {
    ArchiveTestFixture fix;

    const std::string file1_content = "Content of file number one.";
    const std::string file2_content = "File two has different content here.";

    // Write
    {
        auto writer = fix.strategy.create_writer(fix.archive_path);

        writer->write_directory(".", basic_dir_metadata());
        writer->write_directory("subdir", basic_dir_metadata());

        {
            auto& out = writer->begin_file("file1.txt",
                                           file1_content.size(),
                                           0x1234567890ABCDEFULL,
                                           basic_file_metadata());
            out.write(file1_content.data(),
                      static_cast<std::streamsize>(file1_content.size()));
            writer->end_file();
        }

        {
            auto& out = writer->begin_file("subdir/file2.txt",
                                           file2_content.size(),
                                           0xFEDCBA0987654321ULL,
                                           basic_file_metadata());
            out.write(file2_content.data(),
                      static_cast<std::streamsize>(file2_content.size()));
            writer->end_file();
        }

        writer->finish();
    }

    // Read
    auto reader = fix.strategy.create_reader(fix.archive_path);

    int entry_count = 0;
    int dir_count = 0;
    int file_count = 0;

    while (true) {
        auto entry = reader->read_next_entry();
        ++entry_count;

        if (entry.type == ArchiveEntryType::end_of_archive) {
            break;
        }

        if (entry.type == ArchiveEntryType::directory) {
            ++dir_count;
            continue;
        }

        if (entry.type == ArchiveEntryType::regular_file) {
            ++file_count;
            // Consume the file payload before finishing
            auto& payload = reader->current_file_stream();
            std::vector<char> buf(static_cast<std::size_t>(entry.stored_size));
            payload.read(buf.data(), static_cast<std::streamsize>(entry.stored_size));
            reader->finish_file();
        }
    }

    reader->finish();

    REQUIRE(entry_count == 5);  // root + subdir + 2 files + EOA
    REQUIRE(dir_count == 2);
    REQUIRE(file_count == 2);
}

// ===========================================================================
// Codec mismatch detection
// ===========================================================================

TEST_CASE("Archive codec mismatch on read", "[archive]") {
    ArchiveTestFixture fix;

    {
        auto writer = fix.strategy.create_writer(fix.archive_path);
        writer->write_directory(".", basic_dir_metadata());
        writer->finish();
    }

    BinaryArchiveStrategy mismatch_strategy(
        PayloadCodecDescriptor{.compression_name = "huffman",
                               .encryption_name = "none"});

    REQUIRE_THROWS_AS(mismatch_strategy.create_reader(fix.archive_path),
                      std::runtime_error);
}

// ===========================================================================
// Missing root entry
// ===========================================================================

TEST_CASE("Archive finish without root entry throws", "[archive]") {
    ArchiveTestFixture fix;

    auto writer = fix.strategy.create_writer(fix.archive_path);
    // Deliberately skip write_directory(".", ...)
    REQUIRE_THROWS_AS(writer->finish(), std::runtime_error);
}
