#pragma once

#include "strategy/istream_processor.hpp"

namespace backup_system::strategy {

/// 基于 ChaCha20-Poly1305 的对称加密实现。
class ChaCha20Poly1305EncryptionCodec final : public IEncryptionCodec {
public:
    std::string name() const override;
    bool requires_password() const override;
    void encrypt(std::istream& input, std::ostream& output, std::string_view password) const override;
    void decrypt(std::istream& input, std::ostream& output, std::string_view password) const override;
};

}  // namespace backup_system::strategy
