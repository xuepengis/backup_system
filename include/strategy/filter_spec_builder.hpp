#pragma once

#include <string>
#include <vector>

#include "strategy/filter_registry.hpp"

namespace backup_system::strategy {

/// CLI 层原始过滤参数，保留用户输入文本以便集中校验。
struct FilterCliConfig {
    std::vector<std::string> include_paths;
    std::vector<std::string> include_names;
    std::string min_size_text;
    std::string max_size_text;
    std::string modified_after_text;
    std::string modified_before_text;
};

/// 负责将命令行过滤参数转换为可执行的规则描述。
class FilterSpecBuilder {
public:
    /// 判断当前配置中是否实际声明了过滤条件。
    static bool has_filters(const FilterCliConfig& config);

    /// 构建过滤规则描述，并在不允许的模式下阻止使用过滤器。
    static std::vector<FileFilterRuleSpec> build_specs(const FilterCliConfig& config, bool allow_filters);
};

}  // namespace backup_system::strategy
