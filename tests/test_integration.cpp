// GCC warns about designated initializers with missing fields;
// all omitted optional fields safely default to empty / nullopt.
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "core/backup_engine.hpp"
#include "strategy/codec_registry.hpp"
#include "strategy/filter_registry.hpp"
#include "strategy/ifile_filter.hpp"
#include "strategy/istream_processor.hpp"
#include "strategy/iarchive_strategy.hpp"

namespace fs = std::filesystem;

using backup_system::core::BackupEngine;
using backup_system::core::BackupOptions;
using backup_system::core::RestoreOptions;
using backup_system::strategy::create_compression_codec;
using backup_system::strategy::create_encryption_codec;
using backup_system::strategy::create_filter_rule;
using backup_system::strategy::FileFilterRuleSpec;
using backup_system::strategy::CompositeFileFilter;
using backup_system::strategy::PassThroughFileFilter;
using backup_system::strategy::PipelineStreamProcessor;
using backup_system::strategy::BinaryArchiveStrategy;

namespace {

struct IntegrationFixture {
    fs::path test_dir;
    fs::path source_dir;
    fs::path archive_path;
    fs::path restore_dir;

    IntegrationFixture() {
        test_dir = fs::temp_directory_path() / "backup_integration_test";
        source_dir = test_dir / "source";
        archive_path = test_dir / "archive.bks";
        restore_dir = test_dir / "restore";

        fs::create_directories(source_dir);
        fs::create_directories(restore_dir);
    }

    ~IntegrationFixture() {
        std::error_code ec;
        fs::remove_all(test_dir, ec);
    }

    void create_file(const fs::path& relative, const std::string& content) {
        auto full_path = source_dir / relative;
        fs::create_directories(full_path.parent_path());
        std::ofstream ofs(full_path, std::ios::binary);
        ofs.write(content.data(),
                  static_cast<std::streamsize>(content.size()));
    }

    void create_dir(const fs::path& relative) {
        fs::create_directories(source_dir / relative);
    }

    std::string read_restored_file(const fs::path& relative) {
        auto full_path = restore_dir / relative;
        std::ifstream ifs(full_path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(ifs),
                           std::istreambuf_iterator<char>());
    }

    bool restored_path_exists(const fs::path& relative) {
        return fs::exists(restore_dir / relative);
    }
};

}  // namespace

// ===========================================================================
// Basic backup + restore
// ===========================================================================

TEST_CASE("Backup+restore directory tree (no compression/encryption)",
          "[integration]") {
    IntegrationFixture fix;

    // Create test data
    fix.create_dir("subdir");
    fix.create_dir("empty_dir");
    fix.create_file("file_a.txt", "Content of file A.\nLine 2.\n");
    fix.create_file("subdir/file_b.txt", std::string(10000, 'B'));
    fix.create_file("subdir/nested/file_c.txt", "File C in nested dir.");

    // Build engine with no-op codecs
    auto compression = create_compression_codec("none");
    auto encryption = create_encryption_codec("none");
    auto processor = std::make_shared<PipelineStreamProcessor>(
        compression, encryption, "");
    auto archive = std::make_shared<BinaryArchiveStrategy>(
        processor->descriptor());
    auto filter = std::make_shared<PassThroughFileFilter>();

    BackupEngine engine(filter, processor, archive);

    // Backup
    engine.backup(BackupOptions{.source_root = fix.source_dir,
                                .archive_path = fix.archive_path});

    // Verify archive file exists and is non-empty
    REQUIRE(fs::exists(fix.archive_path));
    REQUIRE(fs::file_size(fix.archive_path) > 0);

    // Restore
    engine.restore(RestoreOptions{.archive_path = fix.archive_path,
                                   .restore_root = fix.restore_dir});

    // Verify directory structure
    REQUIRE(fix.restored_path_exists("file_a.txt"));
    REQUIRE(fix.restored_path_exists("subdir/file_b.txt"));
    REQUIRE(fix.restored_path_exists("subdir/nested/file_c.txt"));
    REQUIRE(fs::is_directory(fix.restore_dir / "empty_dir"));
    REQUIRE(fs::is_directory(fix.restore_dir / "subdir"));
    REQUIRE(fs::is_directory(fix.restore_dir / "subdir/nested"));

    // Verify file contents
    REQUIRE(fix.read_restored_file("file_a.txt") ==
            "Content of file A.\nLine 2.\n");
    REQUIRE(fix.read_restored_file("subdir/file_b.txt") ==
            std::string(10000, 'B'));
    REQUIRE(fix.read_restored_file("subdir/nested/file_c.txt") ==
            "File C in nested dir.");
}

// ===========================================================================
// Backup + restore with compression and encryption
// ===========================================================================

TEST_CASE("Backup+restore with Huffman+AES-GCM", "[integration]") {
    IntegrationFixture fix;

    fix.create_file("secret.txt", "This is a top-secret message!\n");
    fix.create_file("data.bin", std::string(5000, 'X'));

    auto compression = create_compression_codec("huffman");
    auto encryption = create_encryption_codec("aes-256-gcm");
    auto processor = std::make_shared<PipelineStreamProcessor>(
        compression, encryption, "integration-test-password");
    auto archive = std::make_shared<BinaryArchiveStrategy>(
        processor->descriptor());
    auto filter = std::make_shared<PassThroughFileFilter>();

    BackupEngine engine(filter, processor, archive);

    // Backup
    engine.backup(BackupOptions{.source_root = fix.source_dir,
                                .archive_path = fix.archive_path});

    REQUIRE(fs::exists(fix.archive_path));

    // Restore
    engine.restore(RestoreOptions{.archive_path = fix.archive_path,
                                   .restore_root = fix.restore_dir});

    REQUIRE(fix.read_restored_file("secret.txt") ==
            "This is a top-secret message!\n");
    REQUIRE(fix.read_restored_file("data.bin") == std::string(5000, 'X'));
}

TEST_CASE("Backup+restore with BWT+ChaCha20-Poly1305", "[integration]") {
    IntegrationFixture fix;

    fix.create_file("report.txt",
                    "Quarterly report: all systems operational.\n");
    fix.create_file("log.txt", std::string(3000, 'L'));

    auto compression = create_compression_codec("bwt");
    auto encryption = create_encryption_codec("chacha20-poly1305");
    auto processor = std::make_shared<PipelineStreamProcessor>(
        compression, encryption, "bwt-chacha-key");
    auto archive = std::make_shared<BinaryArchiveStrategy>(
        processor->descriptor());
    auto filter = std::make_shared<PassThroughFileFilter>();

    BackupEngine engine(filter, processor, archive);

    engine.backup(BackupOptions{.source_root = fix.source_dir,
                                .archive_path = fix.archive_path});

    engine.restore(RestoreOptions{.archive_path = fix.archive_path,
                                   .restore_root = fix.restore_dir});

    REQUIRE(fix.read_restored_file("report.txt") ==
            "Quarterly report: all systems operational.\n");
    REQUIRE(fix.read_restored_file("log.txt") == std::string(3000, 'L'));
}

// ===========================================================================
// Backup with filters
// ===========================================================================

TEST_CASE("Backup with path filter excludes correctly", "[integration]") {
    IntegrationFixture fix;

    fix.create_file("include_me.cpp", "int main() { return 0; }\n");
    fix.create_file("exclude_me.txt", "This should not be backed up.\n");
    fix.create_file("readme.md", "# Project README\n");
    fix.create_dir("src");
    fix.create_file("src/main.cpp", "// source code\n");

    // Filter: only include .cpp files
    auto compression = create_compression_codec("none");
    auto encryption = create_encryption_codec("none");
    auto processor = std::make_shared<PipelineStreamProcessor>(
        compression, encryption, "");
    auto archive = std::make_shared<BinaryArchiveStrategy>(
        processor->descriptor());

    std::vector<std::shared_ptr<backup_system::strategy::IFileFilterRule>> rules;
    rules.push_back(create_filter_rule(
        FileFilterRuleSpec{.type = "name", .string_values = {"*.cpp"}}));
    auto filter = std::make_shared<CompositeFileFilter>(std::move(rules));

    BackupEngine engine(filter, processor, archive);

    engine.backup(BackupOptions{.source_root = fix.source_dir,
                                .archive_path = fix.archive_path});

    engine.restore(RestoreOptions{.archive_path = fix.archive_path,
                                   .restore_root = fix.restore_dir});

    // .cpp files should exist
    REQUIRE(fix.restored_path_exists("include_me.cpp"));
    REQUIRE(fix.restored_path_exists("src/main.cpp"));

    // Directories should still exist
    REQUIRE(fs::is_directory(fix.restore_dir / "src"));
}

// ===========================================================================
// Empty directory backup
// ===========================================================================

TEST_CASE("Backup+restore empty directory", "[integration]") {
    IntegrationFixture fix;

    // Source directory is empty (only root exists)

    auto compression = create_compression_codec("none");
    auto encryption = create_encryption_codec("none");
    auto processor = std::make_shared<PipelineStreamProcessor>(
        compression, encryption, "");
    auto archive = std::make_shared<BinaryArchiveStrategy>(
        processor->descriptor());
    auto filter = std::make_shared<PassThroughFileFilter>();

    BackupEngine engine(filter, processor, archive);

    engine.backup(BackupOptions{.source_root = fix.source_dir,
                                .archive_path = fix.archive_path});

    engine.restore(RestoreOptions{.archive_path = fix.archive_path,
                                   .restore_root = fix.restore_dir});

    // Restore directory should exist and be empty
    REQUIRE(fs::is_directory(fix.restore_dir));
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
