#include "strategy/codec_registry.hpp"

#include "strategy/aes_gcm_codec.hpp"
#include "strategy/bwt_codec.hpp"
#include "strategy/chacha20_poly1305_codec.hpp"
#include "strategy/huffman_codec.hpp"
#include "strategy/ichecksum_engine.hpp"
#include "strategy/lz77_codec.hpp"

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace backup_system::strategy {

namespace {

using CompressionFactory = std::function<std::shared_ptr<ICompressionCodec>()>;
using EncryptionFactory = std::function<std::shared_ptr<IEncryptionCodec>()>;

template <typename FactoryMap>
std::vector<std::string> list_registered_names(const FactoryMap& registry) {
    std::vector<std::string> names;
    names.reserve(registry.size());
    for (const auto& name : registry | std::views::keys) {
        names.push_back(name);
    }
    return names;
}

void copy_stream(std::istream& input, std::ostream& output) {
    std::array<char, 64 * 1024> buffer {};

    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto bytes_read = input.gcount();
        if (bytes_read > 0) {
            output.write(buffer.data(), bytes_read);
        }
    }

    if (!input.eof()) {
        throw std::runtime_error("stream copy failed while reading input");
    }
    if (!output) {
        throw std::runtime_error("stream copy failed while writing output");
    }
}

std::uint64_t seed_from_password(const std::string_view password) {
    std::uint64_t seed = 14695981039346656037ULL;
    for (const auto ch : password) {
        seed ^= static_cast<unsigned char>(ch);
        seed *= 1099511628211ULL;
    }
    return seed == 0 ? 0x9e3779b97f4a7c15ULL : seed;
}

std::uint8_t next_key_byte(std::uint64_t& state) {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return static_cast<std::uint8_t>(state & 0xffU);
}

class NoCompressionCodec final : public ICompressionCodec {
public:
    std::string name() const override;
    void compress(std::istream& input, std::ostream& output) const override;
    void decompress(std::istream& input, std::ostream& output) const override;
};

class RleCompressionCodec final : public ICompressionCodec {
public:
    std::string name() const override;
    void compress(std::istream& input, std::ostream& output) const override;
    void decompress(std::istream& input, std::ostream& output) const override;
};

class NoEncryptionCodec final : public IEncryptionCodec {
public:
    std::string name() const override;
    bool requires_password() const override;
    void encrypt(std::istream& input, std::ostream& output, std::string_view password) const override;
    void decrypt(std::istream& input, std::ostream& output, std::string_view password) const override;
};

class XorStreamEncryptionCodec final : public IEncryptionCodec {
public:
    std::string name() const override;
    bool requires_password() const override;
    void encrypt(std::istream& input, std::ostream& output, std::string_view password) const override;
    void decrypt(std::istream& input, std::ostream& output, std::string_view password) const override;
};

std::string NoCompressionCodec::name() const {
    return "none";
}

void NoCompressionCodec::compress(std::istream& input, std::ostream& output) const {
    copy_stream(input, output);
}

void NoCompressionCodec::decompress(std::istream& input, std::ostream& output) const {
    copy_stream(input, output);
}

std::string RleCompressionCodec::name() const {
    return "rle";
}

void RleCompressionCodec::compress(std::istream& input, std::ostream& output) const {
    int next = input.get();
    if (next == std::char_traits<char>::eof()) {
        return;
    }

    unsigned char current = static_cast<unsigned char>(next);
    std::uint8_t count = 1;
    while ((next = input.get()) != std::char_traits<char>::eof()) {
        const auto byte = static_cast<unsigned char>(next);
        if (byte == current && count < 255) {
            ++count;
            continue;
        }
        output.put(static_cast<char>(count));
        output.put(static_cast<char>(current));
        current = byte;
        count = 1;
    }

    output.put(static_cast<char>(count));
    output.put(static_cast<char>(current));
    if (!input.eof()) {
        throw std::runtime_error("rle compression failed while reading input");
    }
    if (!output) {
        throw std::runtime_error("rle compression failed while writing output");
    }
}

void RleCompressionCodec::decompress(std::istream& input, std::ostream& output) const {
    while (true) {
        const int raw_count = input.get();
        if (raw_count == std::char_traits<char>::eof()) {
            break;
        }
        const int raw_value = input.get();
        if (raw_value == std::char_traits<char>::eof()) {
            throw std::runtime_error("invalid rle stream: truncated run");
        }

        const auto count = static_cast<std::uint8_t>(raw_count);
        const char value = static_cast<char>(raw_value);
        for (std::uint16_t index = 0; index < count; ++index) {
            output.put(value);
        }
    }

    if (!input.eof()) {
        throw std::runtime_error("rle decompression failed while reading input");
    }
    if (!output) {
        throw std::runtime_error("rle decompression failed while writing output");
    }
}

std::string NoEncryptionCodec::name() const {
    return "none";
}

bool NoEncryptionCodec::requires_password() const {
    return false;
}

void NoEncryptionCodec::encrypt(std::istream& input, std::ostream& output, std::string_view password) const {
    (void)password;
    copy_stream(input, output);
}

void NoEncryptionCodec::decrypt(std::istream& input, std::ostream& output, std::string_view password) const {
    (void)password;
    copy_stream(input, output);
}

std::string XorStreamEncryptionCodec::name() const {
    return "xor-stream";
}

bool XorStreamEncryptionCodec::requires_password() const {
    return true;
}

void XorStreamEncryptionCodec::encrypt(std::istream& input,
                                       std::ostream& output,
                                       const std::string_view password) const {
    if (password.empty()) {
        throw std::invalid_argument("xor-stream requires a non-empty password");
    }

    std::uint64_t state = seed_from_password(password);
    std::array<char, 64 * 1024> buffer {};
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto bytes_read = input.gcount();
        for (std::streamsize index = 0; index < bytes_read; ++index) {
            buffer[static_cast<std::size_t>(index)] ^= static_cast<char>(next_key_byte(state));
        }
        if (bytes_read > 0) {
            output.write(buffer.data(), bytes_read);
        }
    }

    if (!input.eof()) {
        throw std::runtime_error("xor-stream transformation failed while reading input");
    }
    if (!output) {
        throw std::runtime_error("xor-stream transformation failed while writing output");
    }
}

void XorStreamEncryptionCodec::decrypt(std::istream& input,
                                       std::ostream& output,
                                       const std::string_view password) const {
    encrypt(input, output, password);
}

const std::map<std::string, CompressionFactory>& compression_registry() {
    static const std::map<std::string, CompressionFactory> registry {
        {"none", [] { return std::make_shared<NoCompressionCodec>(); }},
        {"rle", [] { return std::make_shared<RleCompressionCodec>(); }},
        {"huffman", [] { return std::make_shared<HuffmanCompressionCodec>(); }},
        {"lz77", [] { return std::make_shared<Lz77CompressionCodec>(); }},
        {"bwt", [] { return std::make_shared<BwtCompressionCodec>(); }},
    };
    return registry;
}

const std::map<std::string, EncryptionFactory>& encryption_registry() {
    static const std::map<std::string, EncryptionFactory> registry {
        {"none", [] { return std::make_shared<NoEncryptionCodec>(); }},
        {"aes-256-gcm", [] { return std::make_shared<AesGcmEncryptionCodec>(); }},
        {"chacha20-poly1305", [] { return std::make_shared<ChaCha20Poly1305EncryptionCodec>(); }},
        {"xor-stream", [] { return std::make_shared<XorStreamEncryptionCodec>(); }},
    };
    return registry;
}

}  // namespace

std::shared_ptr<ICompressionCodec> create_compression_codec(const std::string& name) {
    const auto& registry = compression_registry();
    const auto it = registry.find(name);
    if (it == registry.end()) {
        throw std::invalid_argument("unsupported compression codec: " + name);
    }
    return it->second();
}

std::shared_ptr<IEncryptionCodec> create_encryption_codec(const std::string& name) {
    const auto& registry = encryption_registry();
    const auto it = registry.find(name);
    if (it == registry.end()) {
        throw std::invalid_argument("unsupported encryption codec: " + name);
    }
    return it->second();
}

std::vector<std::string> list_compression_codecs() {
    return list_registered_names(compression_registry());
}

std::vector<std::string> list_encryption_codecs() {
    return list_registered_names(encryption_registry());
}

}  // namespace backup_system::strategy
