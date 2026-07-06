#pragma once

#include <filesystem>

namespace backup_system::strategy {

class IFileFilter {
public:
    virtual ~IFileFilter() = default;

    virtual bool should_include(const std::filesystem::directory_entry& entry) const = 0;
};

class PassThroughFileFilter final : public IFileFilter {
public:
    bool should_include(const std::filesystem::directory_entry& entry) const override;
};

}  // namespace backup_system::strategy
