#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <string>

#include "strategy/filter_spec_builder.hpp"

using backup_system::strategy::FilterCliConfig;
using backup_system::strategy::FilterSpecBuilder;

// ===========================================================================
// has_filters
// ===========================================================================

TEST_CASE("has_filters detects filter configuration", "[spec-builder]") {
    SECTION("empty config returns false") {
        FilterCliConfig config;
        REQUIRE_FALSE(FilterSpecBuilder::has_filters(config));
    }

    SECTION("config with include_paths returns true") {
        FilterCliConfig config;
        config.include_paths = {"*.cpp"};
        REQUIRE(FilterSpecBuilder::has_filters(config));
    }

    SECTION("config with include_names returns true") {
        FilterCliConfig config;
        config.include_names = {"*.h"};
        REQUIRE(FilterSpecBuilder::has_filters(config));
    }

    SECTION("config with min_size returns true") {
        FilterCliConfig config;
        config.min_size_text = "1024";
        REQUIRE(FilterSpecBuilder::has_filters(config));
    }

    SECTION("config with max_size returns true") {
        FilterCliConfig config;
        config.max_size_text = "1048576";
        REQUIRE(FilterSpecBuilder::has_filters(config));
    }

    SECTION("config with modified_after returns true") {
        FilterCliConfig config;
        config.modified_after_text = "2025-01-01T00:00:00";
        REQUIRE(FilterSpecBuilder::has_filters(config));
    }

    SECTION("config with modified_before returns true") {
        FilterCliConfig config;
        config.modified_before_text = "2025-12-31T23:59:59";
        REQUIRE(FilterSpecBuilder::has_filters(config));
    }
}

// ===========================================================================
// build_specs
// ===========================================================================

TEST_CASE("build_specs generates correct specs from config", "[spec-builder]") {
    SECTION("empty config produces empty specs") {
        FilterCliConfig config;
        auto specs = FilterSpecBuilder::build_specs(config, true);
        REQUIRE(specs.empty());
    }

    SECTION("path filters generate path specs") {
        FilterCliConfig config;
        config.include_paths = {"docs/*", "src/*.cpp"};
        auto specs = FilterSpecBuilder::build_specs(config, true);
        REQUIRE(specs.size() == 1);
        REQUIRE(specs[0].type == "path");
        REQUIRE(specs[0].string_values.size() == 2);
    }

    SECTION("name filters generate name specs") {
        FilterCliConfig config;
        config.include_names = {"*.log", "*.txt"};
        auto specs = FilterSpecBuilder::build_specs(config, true);
        REQUIRE(specs.size() == 1);
        REQUIRE(specs[0].type == "name");
        REQUIRE(specs[0].string_values.size() == 2);
    }

    SECTION("size filters generate size specs") {
        FilterCliConfig config;
        config.min_size_text = "1024";
        config.max_size_text = "1048576";
        auto specs = FilterSpecBuilder::build_specs(config, true);
        REQUIRE(specs.size() == 1);
        REQUIRE(specs[0].type == "size");
        REQUIRE(specs[0].min_size_bytes.has_value());
        REQUIRE(specs[0].min_size_bytes.value() == 1024);
        REQUIRE(specs[0].max_size_bytes.has_value());
        REQUIRE(specs[0].max_size_bytes.value() == 1048576);
    }

    SECTION("time filters generate time specs") {
        FilterCliConfig config;
        config.modified_after_text = "2025-01-01T00:00:00";
        auto specs = FilterSpecBuilder::build_specs(config, true);
        REQUIRE(specs.size() == 1);
        REQUIRE(specs[0].type == "time");
        REQUIRE(specs[0].modified_after.has_value());
    }

    SECTION("mixed filters generate multiple specs") {
        FilterCliConfig config;
        config.include_paths = {"docs/*"};
        config.min_size_text = "100";
        auto specs = FilterSpecBuilder::build_specs(config, true);
        REQUIRE(specs.size() == 2);
    }
}

TEST_CASE("build_specs rejects invalid filter values", "[spec-builder]") {
    SECTION("non-numeric min_size throws") {
        FilterCliConfig config;
        config.min_size_text = "abc";
        REQUIRE_THROWS_AS(FilterSpecBuilder::build_specs(config, true),
                          std::invalid_argument);
    }

    SECTION("negative min_size throws") {
        FilterCliConfig config;
        config.min_size_text = "-100";
        REQUIRE_THROWS_AS(FilterSpecBuilder::build_specs(config, true),
                          std::invalid_argument);
    }

    SECTION("non-numeric max_size throws") {
        FilterCliConfig config;
        config.max_size_text = "xyz";
        REQUIRE_THROWS_AS(FilterSpecBuilder::build_specs(config, true),
                          std::invalid_argument);
    }

    SECTION("invalid timestamp throws") {
        FilterCliConfig config;
        config.modified_after_text = "not-a-date";
        REQUIRE_THROWS_AS(FilterSpecBuilder::build_specs(config, true),
                          std::invalid_argument);
    }

    SECTION("filters not allowed in restore mode") {
        FilterCliConfig config;
        config.include_paths = {"*.cpp"};
        REQUIRE_THROWS_AS(FilterSpecBuilder::build_specs(config, false),
                          std::invalid_argument);
    }
}
