#pragma once

#include <filesystem>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>

namespace backup_system::strategy {

/// 归档载荷所使用的编解码组合描述。
struct PayloadCodecDescriptor {
    std::string compression_name;
    std::string encryption_name;
};

/// 压缩算法统一接口。
class ICompressionCodec {
public:
    virtual ~ICompressionCodec() = default;

    virtual std::string name() const = 0;
    virtual void compress(std::istream& input, std::ostream& output) const = 0;
    virtual void decompress(std::istream& input, std::ostream& output) const = 0;
};

/// 加密算法统一接口。
class IEncryptionCodec {
public:
    virtual ~IEncryptionCodec() = default;

    virtual std::string name() const = 0;
    virtual bool requires_password() const = 0;
    virtual void encrypt(std::istream& input, std::ostream& output, std::string_view password) const = 0;
    virtual void decrypt(std::istream& input, std::ostream& output, std::string_view password) const = 0;
};

/// 备份/恢复阶段的流处理编排接口。
class IStreamProcessor {
public:
    virtual ~IStreamProcessor() = default;

    virtual PayloadCodecDescriptor descriptor() const = 0;

    virtual void backup(std::istream& input,
                        std::ostream& output,
                        const std::filesystem::path& source_path) const = 0;

    virtual void restore(std::istream& input,
                         std::ostream& output,
                         const std::filesystem::path& archived_path) const = 0;
};

/// 顺序串联压缩与加密能力的默认处理器。
class PipelineStreamProcessor final : public IStreamProcessor {
public:
    PipelineStreamProcessor(std::shared_ptr<ICompressionCodec> compression_codec,
                            std::shared_ptr<IEncryptionCodec> encryption_codec,
                            std::string password);

    PayloadCodecDescriptor descriptor() const override;

    void backup(std::istream& input,
                std::ostream& output,
                const std::filesystem::path& source_path) const override;

    void restore(std::istream& input,
                 std::ostream& output,
                 const std::filesystem::path& archived_path) const override;

private:
    std::shared_ptr<ICompressionCodec> compression_codec_;
    std::shared_ptr<IEncryptionCodec> encryption_codec_;
    std::string password_;
};

}  // namespace backup_system::strategy
