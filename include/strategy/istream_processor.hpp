#pragma once

#include <filesystem>
#include <istream>
#include <ostream>

namespace backup_system::strategy {

class IStreamProcessor {
public:
    virtual ~IStreamProcessor() = default;

    virtual void backup(std::istream& input,
                        std::ostream& output,
                        const std::filesystem::path& source_path) const = 0;

    virtual void restore(std::istream& input,
                         std::ostream& output,
                         const std::filesystem::path& archived_path) const = 0;
};

class DirectCopyStreamProcessor final : public IStreamProcessor {
public:
    void backup(std::istream& input,
                std::ostream& output,
                const std::filesystem::path& source_path) const override;

    void restore(std::istream& input,
                 std::ostream& output,
                 const std::filesystem::path& archived_path) const override;
};

}  // namespace backup_system::strategy
