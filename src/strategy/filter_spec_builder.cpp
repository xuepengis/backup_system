#include "strategy/filter_spec_builder.hpp"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace backup_system::strategy {

namespace {

std::optional<std::uint64_t> parse_size_bytes(const std::string& value) {
    if (value.empty()) {
        return std::nullopt;
    }

    std::size_t parsed_length = 0;
    const auto parsed_value = std::stoull(value, &parsed_length, 10);
    if (parsed_length != value.size()) {
        throw std::invalid_argument("invalid size value: " + value);
    }
    return parsed_value;
}

std::optional<std::chrono::system_clock::time_point> parse_timestamp(const std::string& value) {
    if (value.empty()) {
        return std::nullopt;
    }

    std::tm time_fields {};
    std::istringstream stream(value);
    stream >> std::get_time(&time_fields, "%Y-%m-%dT%H:%M:%S");
    if (stream.fail()) {
        throw std::invalid_argument("invalid timestamp, expected YYYY-MM-DDTHH:MM:SS: " + value);
    }

    const auto time_value = std::mktime(&time_fields);
    if (time_value == -1) {
        throw std::invalid_argument("invalid calendar time: " + value);
    }
    return std::chrono::system_clock::from_time_t(time_value);
}

}  // namespace

bool FilterSpecBuilder::has_filters(const FilterCliConfig& config) {
    return !config.include_paths.empty() ||
           !config.include_names.empty() ||
           !config.min_size_text.empty() ||
           !config.max_size_text.empty() ||
           !config.modified_after_text.empty() ||
           !config.modified_before_text.empty();
}

std::vector<FileFilterRuleSpec> FilterSpecBuilder::build_specs(const FilterCliConfig& config, const bool allow_filters) {
    if (!allow_filters && has_filters(config)) {
        throw std::invalid_argument("file filters are only supported in backup mode");
    }

    const auto min_size_bytes = parse_size_bytes(config.min_size_text);
    const auto max_size_bytes = parse_size_bytes(config.max_size_text);
    const auto modified_after = parse_timestamp(config.modified_after_text);
    const auto modified_before = parse_timestamp(config.modified_before_text);

    if (min_size_bytes && max_size_bytes && *min_size_bytes > *max_size_bytes) {
        throw std::invalid_argument("min-size must not be greater than max-size");
    }
    if (modified_after && modified_before && *modified_after > *modified_before) {
        throw std::invalid_argument("modified-after must not be later than modified-before");
    }

    std::vector<FileFilterRuleSpec> specs;
    if (!config.include_paths.empty()) {
        FileFilterRuleSpec spec;
        spec.type = "path";
        spec.string_values = config.include_paths;
        specs.push_back(std::move(spec));
    }
    if (!config.include_names.empty()) {
        FileFilterRuleSpec spec;
        spec.type = "name";
        spec.string_values = config.include_names;
        specs.push_back(std::move(spec));
    }
    if (min_size_bytes || max_size_bytes) {
        FileFilterRuleSpec spec;
        spec.type = "size";
        spec.min_size_bytes = min_size_bytes;
        spec.max_size_bytes = max_size_bytes;
        specs.push_back(std::move(spec));
    }
    if (modified_after || modified_before) {
        FileFilterRuleSpec spec;
        spec.type = "time";
        spec.modified_after = modified_after;
        spec.modified_before = modified_before;
        specs.push_back(std::move(spec));
    }

    return specs;
}

}  // namespace backup_system::strategy
