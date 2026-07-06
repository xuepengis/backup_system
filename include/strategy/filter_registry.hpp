#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "strategy/ifile_filter.hpp"

namespace backup_system::strategy {

struct FileFilterRuleSpec {
    std::string type;
    std::vector<std::string> string_values;
    std::optional<std::uint64_t> min_size_bytes;
    std::optional<std::uint64_t> max_size_bytes;
    std::optional<std::chrono::system_clock::time_point> modified_after;
    std::optional<std::chrono::system_clock::time_point> modified_before;
};

std::shared_ptr<IFileFilterRule> create_filter_rule(const FileFilterRuleSpec& spec);
std::vector<std::string> list_filter_rule_types();

}  // namespace backup_system::strategy
