#include "strategy/ifile_filter.hpp"

#include <algorithm>
#include <stdexcept>

namespace backup_system::strategy {

bool PassThroughFileFilter::should_include(const std::filesystem::directory_entry& entry,
                                           const std::filesystem::path& source_root) const {
    (void)entry;
    (void)source_root;
    return true;
}

CompositeFileFilter::CompositeFileFilter(std::vector<std::shared_ptr<IFileFilterRule>> rules)
    : rules_(std::move(rules)) {
    if (std::ranges::any_of(rules_, [](const std::shared_ptr<IFileFilterRule>& rule) { return !rule; })) {
        throw std::invalid_argument("filter rules must not contain null entries");
    }
}

bool CompositeFileFilter::should_include(const std::filesystem::directory_entry& entry,
                                         const std::filesystem::path& source_root) const {
    if (entry.is_directory()) {
        return true;
    }
    if (!entry.is_regular_file()) {
        return false;
    }

    return std::ranges::all_of(rules_, [&](const std::shared_ptr<IFileFilterRule>& rule) {
        return rule->matches(entry, source_root);
    });
}

}  // namespace backup_system::strategy
