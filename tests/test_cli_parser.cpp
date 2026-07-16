#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "app/cli_parser.hpp"

using backup_system::app::CliParser;

namespace {

// Helper: convert a vector of strings to argc/argv for CliParser::parse
struct ArgvBuilder {
    std::vector<std::string> args;
    std::vector<char*> argv_ptrs;

    explicit ArgvBuilder(std::vector<std::string> a) : args(std::move(a)) {
        argv_ptrs.reserve(args.size());
        for (auto& s : args) {
            argv_ptrs.push_back(s.data());
        }
    }

    int argc() const { return static_cast<int>(argv_ptrs.size()); }
    char** argv() { return argv_ptrs.data(); }

    auto parse() { return CliParser::parse(argc(), argv()); }
};

}  // namespace

// ===========================================================================
// Valid parsing
// ===========================================================================

TEST_CASE("Parse minimal valid backup", "[cli]") {
    ArgvBuilder builder({"backup_cli", "--mode", "backup", "--src", "/data",
                         "--dest", "/backup.bks"});
    auto opts = builder.parse();

    REQUIRE(opts.mode == "backup");
    REQUIRE(opts.source == "/data");
    REQUIRE(opts.destination == "/backup.bks");
    REQUIRE(opts.compression == "none");
    REQUIRE(opts.encryption == "none");
}

TEST_CASE("Parse minimal valid restore", "[cli]") {
    ArgvBuilder builder({"backup_cli", "--mode", "restore", "--src",
                         "/backup.bks", "--dest", "/restore_out"});
    auto opts = builder.parse();

    REQUIRE(opts.mode == "restore");
    REQUIRE(opts.source == "/backup.bks");
    REQUIRE(opts.destination == "/restore_out");
}

TEST_CASE("Parse all options including filters", "[cli]") {
    ArgvBuilder builder({"backup_cli",
                         "--mode", "backup",
                         "--src", "/data",
                         "--dest", "/out.bks",
                         "--compression", "huffman",
                         "--encryption", "aes-256-gcm",
                         "--password", "secret",
                         "--include-path", "docs/*",
                         "--include-path", "src/*.cpp",
                         "--include-name", "*.log",
                         "--min-size", "1024",
                         "--max-size", "1048576",
                         "--modified-after", "2025-01-01T00:00:00"});

    auto opts = builder.parse();

    REQUIRE(opts.mode == "backup");
    REQUIRE(opts.compression == "huffman");
    REQUIRE(opts.encryption == "aes-256-gcm");
    REQUIRE(opts.password == "secret");
    REQUIRE(opts.filter_config.include_paths.size() == 2);
    REQUIRE(opts.filter_config.include_paths[0] == "docs/*");
    REQUIRE(opts.filter_config.include_paths[1] == "src/*.cpp");
    REQUIRE(opts.filter_config.include_names.size() == 1);
    REQUIRE(opts.filter_config.include_names[0] == "*.log");
    REQUIRE(opts.filter_config.min_size_text == "1024");
    REQUIRE(opts.filter_config.max_size_text == "1048576");
    REQUIRE(opts.filter_config.modified_after_text == "2025-01-01T00:00:00");
}

// ===========================================================================
// Help
// ===========================================================================

TEST_CASE("--help throws __print_usage__ sentinel", "[cli]") {
    SECTION("long form --help") {
        ArgvBuilder builder({"backup_cli", "--help"});
        REQUIRE_THROWS_AS(builder.parse(), std::invalid_argument);
        try {
            builder.parse();
        } catch (const std::invalid_argument& e) {
            REQUIRE(std::string(e.what()) == "__print_usage__");
        }
    }

    SECTION("short form -h") {
        ArgvBuilder builder({"backup_cli", "-h"});
        REQUIRE_THROWS_AS(builder.parse(), std::invalid_argument);
        try {
            builder.parse();
        } catch (const std::invalid_argument& e) {
            REQUIRE(std::string(e.what()) == "__print_usage__");
        }
    }
}

// ===========================================================================
// Error handling
// ===========================================================================

TEST_CASE("Missing required arguments throws", "[cli]") {
    SECTION("no arguments at all") {
        ArgvBuilder builder({"backup_cli"});
        REQUIRE_THROWS_AS(builder.parse(), std::invalid_argument);
    }

    SECTION("missing --dest") {
        ArgvBuilder builder({"backup_cli", "--mode", "backup", "--src", "/a"});
        REQUIRE_THROWS_AS(builder.parse(), std::invalid_argument);
    }

    SECTION("missing --mode") {
        ArgvBuilder builder({"backup_cli", "--src", "/a", "--dest", "/b"});
        REQUIRE_THROWS_AS(builder.parse(), std::invalid_argument);
    }
}

TEST_CASE("Unknown argument throws", "[cli]") {
    ArgvBuilder builder({"backup_cli", "--invalid-flag"});
    REQUIRE_THROWS_AS(builder.parse(), std::invalid_argument);
}

TEST_CASE("Missing value for argument throws", "[cli]") {
    ArgvBuilder builder({"backup_cli", "--mode"});
    REQUIRE_THROWS_AS(builder.parse(), std::invalid_argument);
}

TEST_CASE("Invalid mode throws", "[cli]") {
    ArgvBuilder builder(
        {"backup_cli", "--mode", "sync", "--src", "/a", "--dest", "/b"});
    REQUIRE_THROWS_AS(builder.parse(), std::invalid_argument);
}

TEST_CASE("Password without encryption throws", "[cli]") {
    ArgvBuilder builder({"backup_cli", "--mode", "backup", "--src", "/a",
                         "--dest", "/b", "--password", "secret"});
    REQUIRE_THROWS_AS(builder.parse(), std::invalid_argument);
}

TEST_CASE("Encryption without password throws", "[cli]") {
    ArgvBuilder builder({"backup_cli", "--mode", "backup", "--src", "/a",
                         "--dest", "/b", "--encryption", "aes-256-gcm"});
    REQUIRE_THROWS_AS(builder.parse(), std::invalid_argument);
}

// ===========================================================================
// usage()
// ===========================================================================

TEST_CASE("usage returns formatted help string", "[cli]") {
    auto usage_text = CliParser::usage("backup_cli");

    REQUIRE_FALSE(usage_text.empty());
    REQUIRE(usage_text.find("Usage:") != std::string::npos);
    REQUIRE(usage_text.find("--mode") != std::string::npos);
    REQUIRE(usage_text.find("backup") != std::string::npos);
    REQUIRE(usage_text.find("restore") != std::string::npos);
}
