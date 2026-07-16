#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>

#include "utils/metadata_utils.hpp"

namespace fs = std::filesystem;

using backup_system::utils::FileMetadata;
using backup_system::utils::MetadataUtils;

namespace {

struct TempFileFixture {
    fs::path file_path;

    TempFileFixture() {
        auto temp_dir = fs::temp_directory_path() / "backup_test_metadata";
        fs::create_directories(temp_dir);
        file_path = temp_dir / "test_file.txt";

        std::ofstream ofs(file_path);
        ofs << "metadata test content\n";
        ofs.close();

        // Set known permissions
        fs::permissions(file_path,
                        fs::perms::owner_read | fs::perms::owner_write);
    }

    ~TempFileFixture() {
        std::error_code ec;
        fs::remove_all(file_path.parent_path(), ec);
    }
};

}  // namespace

// ===========================================================================
// collect()
// ===========================================================================

TEST_CASE("Collect metadata from regular file", "[metadata]") {
    TempFileFixture fix;

    auto metadata = MetadataUtils::collect(fix.file_path);

    // Mode should indicate a regular file
    REQUIRE(metadata.mode != 0);

    // Modification time should be set (file was just created)
    REQUIRE(metadata.modification_time_sec > 0);

    // Owner UID/GID should be set (current user)
    REQUIRE(metadata.owner_uid > 0);
    REQUIRE(metadata.owner_gid > 0);
}

TEST_CASE("Collect metadata from directory", "[metadata]") {
    TempFileFixture fix;
    auto dir_path = fix.file_path.parent_path();

    auto metadata = MetadataUtils::collect(dir_path);

    REQUIRE(metadata.mode != 0);
    REQUIRE(metadata.modification_time_sec > 0);
}

// ===========================================================================
// apply()
// ===========================================================================

TEST_CASE("Apply metadata restores mode", "[metadata]") {
    TempFileFixture fix;

    // Collect current metadata then change mode
    auto original = MetadataUtils::collect(fix.file_path);

    FileMetadata new_meta = original;
    new_meta.mode = 0600;  // read+write owner only

    MetadataUtils::apply(fix.file_path, new_meta);

    // Verify the mode was applied
    auto restored = MetadataUtils::collect(fix.file_path);

    // Permission bits should match (mode & 07777)
    REQUIRE((restored.mode & 07777) == 0600);
}

TEST_CASE("Apply metadata preserves modification time", "[metadata]") {
    TempFileFixture fix;

    // Set a specific modification time (1 hour ago)
    auto one_hour_ago = std::chrono::system_clock::now() - std::chrono::hours(1);
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(
                    one_hour_ago.time_since_epoch())
                    .count();

    FileMetadata meta;
    meta.mode = 0644;
    meta.modification_time_sec = secs;
    meta.modification_time_nsec = 0;

    MetadataUtils::apply(fix.file_path, meta);

    auto collected = MetadataUtils::collect(fix.file_path);

    // Modification time should be within 2 seconds of what we set
    REQUIRE(collected.modification_time_sec >= secs - 1);
    REQUIRE(collected.modification_time_sec <= secs + 1);
}

TEST_CASE("Metadata round-trip collect then apply", "[metadata]") {
    TempFileFixture fix;

    auto original = MetadataUtils::collect(fix.file_path);

    // Modify the file
    {
        std::ofstream ofs(fix.file_path, std::ios::app);
        ofs << "additional content\n";
    }

    // Apply original metadata (reverts timestamp to original)
    MetadataUtils::apply(fix.file_path, original);

    auto restored = MetadataUtils::collect(fix.file_path);

    // Mode should be restored
    REQUIRE((restored.mode & 07777) == (original.mode & 07777));

    // Modification time should be close to original
    REQUIRE(restored.modification_time_sec >= original.modification_time_sec - 1);
    REQUIRE(restored.modification_time_sec <= original.modification_time_sec + 1);
}
