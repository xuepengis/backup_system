#include "strategy/filter_registry.hpp"

#include <algorithm>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <system_error>

#include "utils/path_utils.hpp"

namespace backup_system::strategy {

namespace {

using FilterRuleFactory = std::function<std::shared_ptr<IFileFilterRule>(const FileFilterRuleSpec&)>;

bool wildcard_match(std::string_view pattern, std::string_view value) {
    std::size_t pattern_index = 0;
    std::size_t value_index = 0;
    std::size_t star_index = std::string_view::npos;
    std::size_t match_index = 0;

    while (value_index < value.size()) {
        if (pattern_index < pattern.size() &&
            (pattern[pattern_index] == '?' || pattern[pattern_index] == value[value_index])) {
            ++pattern_index;
            ++value_index;
            continue;
        }

        if (pattern_index < pattern.size() && pattern[pattern_index] == '*') {
            star_index = pattern_index++;
            match_index = value_index;
            continue;
        }

        if (star_index != std::string_view::npos) {
            pattern_index = star_index + 1;
            value_index = ++match_index;
            continue;
        }

        return false;
    }

    while (pattern_index < pattern.size() && pattern[pattern_index] == '*') {
        ++pattern_index;
    }

    return pattern_index == pattern.size();
}

bool matches_any_pattern(const std::vector<std::string>& patterns, const std::string_view value) {
    return std::ranges::any_of(patterns, [&](const std::string& pattern) {
        return wildcard_match(pattern, value);
    });
}

std::chrono::system_clock::time_point to_system_time(const std::filesystem::file_time_type file_time) {
    return std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        file_time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
}

class PathPatternRule final : public IFileFilterRule {
public:
    explicit PathPatternRule(std::vector<std::string> patterns)
        : patterns_(std::move(patterns)) {
        if (patterns_.empty()) {
            throw std::invalid_argument("path rule requires at least one pattern");
        }
    }

    std::string name() const override { return "path"; }

    bool matches(const std::filesystem::directory_entry& entry,
                 const std::filesystem::path& source_root) const override {
        const auto relative_path =
            utils::PathUtils::normalize_for_storage(std::filesystem::relative(entry.path(), source_root));
        return matches_any_pattern(patterns_, utils::PathUtils::to_generic_string(relative_path));
    }

private:
    std::vector<std::string> patterns_;
};

class NamePatternRule final : public IFileFilterRule {
public:
    explicit NamePatternRule(std::vector<std::string> patterns)
        : patterns_(std::move(patterns)) {
        if (patterns_.empty()) {
            throw std::invalid_argument("name rule requires at least one pattern");
        }
    }

    std::string name() const override { return "name"; }

    bool matches(const std::filesystem::directory_entry& entry,
                 const std::filesystem::path& source_root) const override {
        (void)source_root;
        return matches_any_pattern(patterns_, entry.path().filename().string());
    }

private:
    std::vector<std::string> patterns_;
};

class SizeRangeRule final : public IFileFilterRule {
public:
    SizeRangeRule(std::optional<std::uint64_t> min_size_bytes, std::optional<std::uint64_t> max_size_bytes)
        : min_size_bytes_(min_size_bytes),
          max_size_bytes_(max_size_bytes) {
        if (!min_size_bytes_ && !max_size_bytes_) {
            throw std::invalid_argument("size rule requires min-size or max-size");
        }
        if (min_size_bytes_ && max_size_bytes_ && *min_size_bytes_ > *max_size_bytes_) {
            throw std::invalid_argument("min-size must not be greater than max-size");
        }
    }

    std::string name() const override { return "size"; }

    bool matches(const std::filesystem::directory_entry& entry,
                 const std::filesystem::path& source_root) const override {
        (void)source_root;
        std::error_code error_code;
        const auto file_size = entry.file_size(error_code);
        if (error_code) {
            throw std::runtime_error("failed to query file size for filter evaluation: " + entry.path().string());
        }

        if (min_size_bytes_ && file_size < *min_size_bytes_) {
            return false;
        }
        if (max_size_bytes_ && file_size > *max_size_bytes_) {
            return false;
        }
        return true;
    }

private:
    std::optional<std::uint64_t> min_size_bytes_;
    std::optional<std::uint64_t> max_size_bytes_;
};

class ModifiedTimeRule final : public IFileFilterRule {
public:
    ModifiedTimeRule(std::optional<std::chrono::system_clock::time_point> modified_after,
                     std::optional<std::chrono::system_clock::time_point> modified_before)
        : modified_after_(modified_after),
          modified_before_(modified_before) {
        if (!modified_after_ && !modified_before_) {
            throw std::invalid_argument("time rule requires modified-after or modified-before");
        }
        if (modified_after_ && modified_before_ && *modified_after_ > *modified_before_) {
            throw std::invalid_argument("modified-after must not be later than modified-before");
        }
    }

    std::string name() const override { return "time"; }

    bool matches(const std::filesystem::directory_entry& entry,
                 const std::filesystem::path& source_root) const override {
        (void)source_root;
        std::error_code error_code;
        const auto write_time = entry.last_write_time(error_code);
        if (error_code) {
            throw std::runtime_error("failed to query file time for filter evaluation: " + entry.path().string());
        }

        const auto modified_time = to_system_time(write_time);
        if (modified_after_ && modified_time < *modified_after_) {
            return false;
        }
        if (modified_before_ && modified_time > *modified_before_) {
            return false;
        }
        return true;
    }

private:
    std::optional<std::chrono::system_clock::time_point> modified_after_;
    std::optional<std::chrono::system_clock::time_point> modified_before_;
};

const std::map<std::string, FilterRuleFactory>& filter_registry() {
    static const std::map<std::string, FilterRuleFactory> registry {
        {"path", [](const FileFilterRuleSpec& spec) {
             return std::make_shared<PathPatternRule>(spec.string_values);
         }},
        {"name", [](const FileFilterRuleSpec& spec) {
             return std::make_shared<NamePatternRule>(spec.string_values);
         }},
        {"size", [](const FileFilterRuleSpec& spec) {
             return std::make_shared<SizeRangeRule>(spec.min_size_bytes, spec.max_size_bytes);
         }},
        {"time", [](const FileFilterRuleSpec& spec) {
             return std::make_shared<ModifiedTimeRule>(spec.modified_after, spec.modified_before);
         }},
    };
    return registry;
}

}  // namespace

std::shared_ptr<IFileFilterRule> create_filter_rule(const FileFilterRuleSpec& spec) {
    const auto& registry = filter_registry();
    const auto it = registry.find(spec.type);
    if (it == registry.end()) {
        throw std::invalid_argument("unsupported filter rule type: " + spec.type);
    }
    return it->second(spec);
}

std::vector<std::string> list_filter_rule_types() {
    std::vector<std::string> names;
    names.reserve(filter_registry().size());
    for (const auto& name : filter_registry() | std::views::keys) {
        names.push_back(name);
    }
    return names;
}

}  // namespace backup_system::strategy
