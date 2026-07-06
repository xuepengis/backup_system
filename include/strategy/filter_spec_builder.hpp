#pragma once

#include <string>
#include <vector>

#include "strategy/filter_registry.hpp"

namespace backup_system::strategy {

struct FilterCliConfig {
    std::vector<std::string> include_paths;
    std::vector<std::string> include_names;
    std::string min_size_text;
    std::string max_size_text;
    std::string modified_after_text;
    std::string modified_before_text;
};

class FilterSpecBuilder {
public:
    static bool has_filters(const FilterCliConfig& config);
    static std::vector<FileFilterRuleSpec> build_specs(const FilterCliConfig& config, bool allow_filters);
};

}  // namespace backup_system::strategy
