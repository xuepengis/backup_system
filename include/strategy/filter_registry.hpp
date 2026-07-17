#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "strategy/ifile_filter.hpp"

namespace backup_system::strategy {

/// 文件过滤规则的标准化描述，供注册表创建具体规则对象。
struct FileFilterRuleSpec {
    /// 规则类型，例如 `path`、`name`、`size`、`modified-time`。
    std::string type;
    /// 通用字符串参数，通常用于通配符模式。
    std::vector<std::string> string_values;
    /// 最小文件大小限制。
    std::optional<std::uint64_t> min_size_bytes;
    /// 最大文件大小限制。
    std::optional<std::uint64_t> max_size_bytes;
    /// 修改时间下界。
    std::optional<std::chrono::system_clock::time_point> modified_after;
    /// 修改时间上界。
    std::optional<std::chrono::system_clock::time_point> modified_before;
};

/// 根据规则描述创建具体过滤规则。
std::shared_ptr<IFileFilterRule> create_filter_rule(const FileFilterRuleSpec& spec);

/// 返回当前支持的过滤规则类型列表。
std::vector<std::string> list_filter_rule_types();

}  // namespace backup_system::strategy
