// GCC warns about designated initializers with missing fields;
// all omitted optional fields safely default to empty / nullopt.
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "strategy/codec_registry.hpp"
#include "strategy/filter_registry.hpp"

using backup_system::strategy::create_compression_codec;
using backup_system::strategy::create_encryption_codec;
using backup_system::strategy::create_filter_rule;
using backup_system::strategy::list_compression_codecs;
using backup_system::strategy::list_encryption_codecs;
using backup_system::strategy::list_filter_rule_types;
using backup_system::strategy::FileFilterRuleSpec;

// ===========================================================================
// Compression codec registry
// ===========================================================================

TEST_CASE("create_compression_codec valid names", "[registry]") {
    const std::vector<std::string> expected = {"none", "rle", "huffman", "lz77", "bwt"};

    for (const auto& name : expected) {
        INFO("Testing compression codec: " << name);
        auto codec = create_compression_codec(name);
        REQUIRE(codec != nullptr);
        REQUIRE(codec->name() == name);
    }
}

TEST_CASE("create_compression_codec invalid name throws", "[registry]") {
    SECTION("unknown algorithm") {
        REQUIRE_THROWS_AS(create_compression_codec("gzip"),
                          std::invalid_argument);
    }

    SECTION("empty string") {
        REQUIRE_THROWS_AS(create_compression_codec(""),
                          std::invalid_argument);
    }
}

// ===========================================================================
// Encryption codec registry
// ===========================================================================

TEST_CASE("create_encryption_codec valid names", "[registry]") {
    const std::vector<std::string> expected = {"none", "xor-stream", "aes-256-gcm",
                                               "chacha20-poly1305"};

    for (const auto& name : expected) {
        INFO("Testing encryption codec: " << name);
        auto codec = create_encryption_codec(name);
        REQUIRE(codec != nullptr);
        REQUIRE(codec->name() == name);
    }
}

TEST_CASE("create_encryption_codec invalid name throws", "[registry]") {
    SECTION("unknown algorithm") {
        REQUIRE_THROWS_AS(create_encryption_codec("rot13"),
                          std::invalid_argument);
    }

    SECTION("empty string") {
        REQUIRE_THROWS_AS(create_encryption_codec(""),
                          std::invalid_argument);
    }
}

// ===========================================================================
// List functions
// ===========================================================================

TEST_CASE("list_compression_codecs returns all registered names", "[registry]") {
    auto names = list_compression_codecs();
    REQUIRE(names.size() >= 5);

    auto contains = [&](const std::string& name) {
        return std::find(names.begin(), names.end(), name) != names.end();
    };

    REQUIRE(contains("none"));
    REQUIRE(contains("rle"));
    REQUIRE(contains("huffman"));
    REQUIRE(contains("lz77"));
    REQUIRE(contains("bwt"));
}

TEST_CASE("list_encryption_codecs returns all registered names", "[registry]") {
    auto names = list_encryption_codecs();
    REQUIRE(names.size() >= 4);

    auto contains = [&](const std::string& name) {
        return std::find(names.begin(), names.end(), name) != names.end();
    };

    REQUIRE(contains("none"));
    REQUIRE(contains("xor-stream"));
    REQUIRE(contains("aes-256-gcm"));
    REQUIRE(contains("chacha20-poly1305"));
}

TEST_CASE("list_filter_rule_types returns all registered types", "[registry]") {
    auto names = list_filter_rule_types();
    REQUIRE(names.size() == 4);

    auto contains = [&](const std::string& name) {
        return std::find(names.begin(), names.end(), name) != names.end();
    };
    REQUIRE(contains("path"));
    REQUIRE(contains("name"));
    REQUIRE(contains("size"));
    REQUIRE(contains("time"));
}

// ===========================================================================
// Filter rule registry
// ===========================================================================

TEST_CASE("create_filter_rule for all registered types", "[registry]") {
    SECTION("path rule") {
        auto rule = create_filter_rule(
            FileFilterRuleSpec{.type = "path", .string_values = {"*.cpp"}});
        REQUIRE(rule != nullptr);
        REQUIRE(rule->name() == "path");
    }

    SECTION("name rule") {
        auto rule = create_filter_rule(
            FileFilterRuleSpec{.type = "name", .string_values = {"*.h"}});
        REQUIRE(rule != nullptr);
        REQUIRE(rule->name() == "name");
    }

    SECTION("size rule") {
        auto rule = create_filter_rule(
            FileFilterRuleSpec{.type = "size", .min_size_bytes = 1024});
        REQUIRE(rule != nullptr);
        REQUIRE(rule->name() == "size");
    }

    SECTION("time rule") {
        auto rule = create_filter_rule(
            FileFilterRuleSpec{.type = "time",
                               .modified_after = std::chrono::system_clock::now()});
        REQUIRE(rule != nullptr);
        REQUIRE(rule->name() == "time");
    }

    SECTION("unknown type throws") {
        REQUIRE_THROWS_AS(
            create_filter_rule(FileFilterRuleSpec{.type = "unknown"}),
            std::invalid_argument);
    }
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
