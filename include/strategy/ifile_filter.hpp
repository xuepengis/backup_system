#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace backup_system::strategy {

/// 文件过滤器接口，用于决定目录遍历项是否参与备份。
class IFileFilter {
public:
    virtual ~IFileFilter() = default;

    virtual bool should_include(const std::filesystem::directory_entry& entry,
                                const std::filesystem::path& source_root) const = 0;
};

/// 原子过滤规则接口，供组合过滤器复用。
class IFileFilterRule {
public:
    virtual ~IFileFilterRule() = default;

    virtual std::string name() const = 0;
    virtual bool matches(const std::filesystem::directory_entry& entry,
                         const std::filesystem::path& source_root) const = 0;
};

/// 透传过滤器，不对文件做任何筛选。
class PassThroughFileFilter final : public IFileFilter {
public:
    bool should_include(const std::filesystem::directory_entry& entry,
                        const std::filesystem::path& source_root) const override;
};

/// 文件过滤配置聚合对象。
struct FileFilterOptions {
    std::vector<std::string> path_patterns;
    std::vector<std::string> name_patterns;
    std::optional<std::uint64_t> min_size_bytes;
    std::optional<std::uint64_t> max_size_bytes;
    std::optional<std::chrono::system_clock::time_point> modified_after;
    std::optional<std::chrono::system_clock::time_point> modified_before;
};

/// 组合过滤器，要求所有规则同时满足。
class CompositeFileFilter final : public IFileFilter {
public:
    explicit CompositeFileFilter(std::vector<std::shared_ptr<IFileFilterRule>> rules);

    bool should_include(const std::filesystem::directory_entry& entry,
                        const std::filesystem::path& source_root) const override;

private:
    std::vector<std::shared_ptr<IFileFilterRule>> rules_;
};

}  // namespace backup_system::strategy
