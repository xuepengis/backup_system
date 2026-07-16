#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <stdexcept>

#include "utils/path_utils.hpp"

namespace fs = std::filesystem;

using backup_system::utils::PathUtils;

// ===========================================================================
// normalize_for_storage
// ===========================================================================

TEST_CASE("normalize_for_storage handles valid relative paths", "[path]") {
    SECTION("simple path unchanged") {
        auto result = PathUtils::normalize_for_storage("foo/bar");
        REQUIRE(result == "foo/bar");
    }

    SECTION("dot segments removed") {
        auto result = PathUtils::normalize_for_storage("foo/./bar");
        REQUIRE(result == "foo/bar");
    }

    SECTION("single dot") {
        auto result = PathUtils::normalize_for_storage(".");
        REQUIRE(result == ".");
    }

    SECTION("trailing slash normalized") {
        auto result = PathUtils::normalize_for_storage("foo/bar/");
        REQUIRE(result == "foo/bar/");
    }

    SECTION("double separator removed") {
        auto result = PathUtils::normalize_for_storage("foo//bar");
        REQUIRE(result == "foo/bar");
    }

    SECTION("simple filename") {
        auto result = PathUtils::normalize_for_storage("file.txt");
        REQUIRE(result == "file.txt");
    }
}

TEST_CASE("normalize_for_storage rejects absolute paths", "[path]") {
    SECTION("Unix absolute path") {
        REQUIRE_THROWS_AS(PathUtils::normalize_for_storage("/etc/passwd"),
                          std::invalid_argument);
    }

    SECTION("root path") {
        REQUIRE_THROWS_AS(PathUtils::normalize_for_storage("/"),
                          std::invalid_argument);
    }
}

TEST_CASE("normalize_for_storage rejects path escape", "[path]") {
    SECTION("parent directory escape") {
        REQUIRE_THROWS_AS(PathUtils::normalize_for_storage("../escape"),
                          std::invalid_argument);
    }

    SECTION("mid-path escape") {
        REQUIRE_THROWS_AS(PathUtils::normalize_for_storage("foo/../../etc"),
                          std::invalid_argument);
    }

    SECTION("trailing parent") {
        REQUIRE_THROWS_AS(PathUtils::normalize_for_storage("foo/bar/.."),
                          std::invalid_argument);
    }
}

TEST_CASE("normalize_for_storage handles empty path", "[path]") {
    SECTION("empty string") {
        auto result = PathUtils::normalize_for_storage("");
        REQUIRE(result.empty());
    }

    SECTION("default constructed path") {
        auto result = PathUtils::normalize_for_storage(fs::path{});
        REQUIRE(result.empty());
    }
}

// ===========================================================================
// to_generic_string
// ===========================================================================

TEST_CASE("to_generic_string converts to forward-slash format", "[path]") {
    SECTION("simple path") {
        auto result = PathUtils::to_generic_string("foo/bar");
        REQUIRE(result == "foo/bar");
    }

    SECTION("single component") {
        auto result = PathUtils::to_generic_string("file.txt");
        REQUIRE(result == "file.txt");
    }

    SECTION("dot path") {
        auto result = PathUtils::to_generic_string(".");
        REQUIRE(result == ".");
    }

    SECTION("nested path") {
        auto result = PathUtils::to_generic_string("a/b/c/d");
        REQUIRE(result == "a/b/c/d");
    }

    SECTION("rejects invalid path") {
        REQUIRE_THROWS_AS(PathUtils::to_generic_string("/absolute"),
                          std::invalid_argument);
    }
}

// ===========================================================================
// from_generic_string
// ===========================================================================

TEST_CASE("from_generic_string converts generic string to path", "[path]") {
    SECTION("simple path") {
        auto result = PathUtils::from_generic_string("foo/bar");
        REQUIRE(result == "foo/bar");
    }

    SECTION("empty string returns empty path") {
        auto result = PathUtils::from_generic_string("");
        REQUIRE(result.empty());
    }

    SECTION("single file") {
        auto result = PathUtils::from_generic_string("readme.md");
        REQUIRE(result == "readme.md");
    }

    SECTION("rejects path with parent directory") {
        REQUIRE_THROWS_AS(PathUtils::from_generic_string("../escape"),
                          std::invalid_argument);
    }
}

TEST_CASE("to_generic_string and from_generic_string round-trip", "[path]") {
    SECTION("nested relative path") {
        const std::string original = "projects/backup/src/main.cpp";
        auto path = PathUtils::from_generic_string(original);
        auto back_to_string = PathUtils::to_generic_string(path);
        REQUIRE(back_to_string == original);
    }

    SECTION("dot path") {
        const std::string original = ".";
        auto path = PathUtils::from_generic_string(original);
        auto back_to_string = PathUtils::to_generic_string(path);
        REQUIRE(back_to_string == original);
    }
}
