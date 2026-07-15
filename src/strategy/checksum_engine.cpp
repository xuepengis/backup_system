#include "strategy/ichecksum_engine.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <istream>
#include <map>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

namespace backup_system::strategy {

namespace {

// ---------------------------------------------------------------------------
// FNV-1a constants
// ---------------------------------------------------------------------------
constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

// ---------------------------------------------------------------------------
// CRC-32 constants (IEEE 802.3 / polynomial 0xEDB88320)
// ---------------------------------------------------------------------------
constexpr std::uint32_t kCrc32Polynomial = 0xEDB88320;

constexpr std::array<std::uint32_t, 256> generate_crc32_table() {
    std::array<std::uint32_t, 256> table {};
    for (std::uint32_t index = 0; index < 256; ++index) {
        std::uint32_t crc = index;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ ((crc & 1) != 0 ? kCrc32Polynomial : 0);
        }
        table[index] = crc;
    }
    return table;
}

constexpr std::array<std::uint32_t, 256> kCrc32Table = generate_crc32_table();

// ---------------------------------------------------------------------------
// Shared helpers
// ---------------------------------------------------------------------------

/// One-shot: read an entire stream, feed it through the engine, return the
/// final checksum value.  Follows the 64 KiB chunk pattern used by all codecs.
std::uint64_t compute_from_stream(std::istream& input, const IChecksumEngine& engine) {
    std::array<char, 64 * 1024> buffer {};
    auto state = engine.initial_value();

    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto bytes_read = input.gcount();
        if (bytes_read > 0) {
            engine.update(state, buffer.data(), static_cast<std::size_t>(bytes_read));
        }
    }

    if (!input.eof()) {
        throw std::runtime_error("checksum computation failed while reading input");
    }
    return engine.finalize(state);
}

// ---------------------------------------------------------------------------
// Factory registry (same pattern as compression / encryption registries)
// ---------------------------------------------------------------------------

using ChecksumFactory = std::function<std::shared_ptr<IChecksumEngine>()>;

const std::map<std::string, ChecksumFactory>& checksum_registry() {
    static const std::map<std::string, ChecksumFactory> registry {
        {"fnv1a", [] { return std::make_shared<Fnv1aChecksumEngine>(); }},
        {"crc32", [] { return std::make_shared<Crc32ChecksumEngine>(); }},
    };
    return registry;
}

}  // namespace

// ===================================================================
// Fnv1aChecksumEngine
// ===================================================================

std::string Fnv1aChecksumEngine::name() const {
    return "fnv1a";
}

std::uint64_t Fnv1aChecksumEngine::compute_stream(std::istream& input) const {
    return compute_from_stream(input, *this);
}

std::uint64_t Fnv1aChecksumEngine::compute_file(const std::filesystem::path& path) const {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to open file for checksum: " + path.string());
    }
    return compute_stream(input);
}

std::uint64_t Fnv1aChecksumEngine::initial_value() const {
    return kFnvOffsetBasis;
}

void Fnv1aChecksumEngine::update(std::uint64_t& state,
                                  const char* data,
                                  const std::size_t size) const {
    for (std::size_t index = 0; index < size; ++index) {
        state ^= static_cast<unsigned char>(data[index]);
        state *= kFnvPrime;
    }
}

std::uint64_t Fnv1aChecksumEngine::finalize(std::uint64_t state) const {
    return state;
}

// ===================================================================
// Crc32ChecksumEngine
// ===================================================================

std::string Crc32ChecksumEngine::name() const {
    return "crc32";
}

std::uint64_t Crc32ChecksumEngine::compute_stream(std::istream& input) const {
    return compute_from_stream(input, *this);
}

std::uint64_t Crc32ChecksumEngine::compute_file(const std::filesystem::path& path) const {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to open file for checksum: " + path.string());
    }
    return compute_stream(input);
}

std::uint64_t Crc32ChecksumEngine::initial_value() const {
    return 0xFFFFFFFF;
}

void Crc32ChecksumEngine::update(std::uint64_t& state,
                                  const char* data,
                                  const std::size_t size) const {
    auto crc = static_cast<std::uint32_t>(state);
    for (std::size_t index = 0; index < size; ++index) {
        crc = (crc >> 8) ^ kCrc32Table[(crc ^ static_cast<unsigned char>(data[index])) & 0xFF];
    }
    state = crc;
}

std::uint64_t Crc32ChecksumEngine::finalize(std::uint64_t state) const {
    return (static_cast<std::uint32_t>(state) ^ 0xFFFFFFFF);
}

// ===================================================================
// Factory + registry
// ===================================================================

std::shared_ptr<IChecksumEngine> create_checksum_engine(const std::string& name) {
    const auto& registry = checksum_registry();
    const auto it = registry.find(name);
    if (it == registry.end()) {
        throw std::invalid_argument("unsupported checksum engine: " + name);
    }
    return it->second();
}

std::vector<std::string> list_checksum_engines() {
    std::vector<std::string> names;
    names.reserve(checksum_registry().size());
    for (const auto& name : checksum_registry() | std::views::keys) {
        names.push_back(name);
    }
    return names;
}

// ===================================================================
// Flag conversion helpers
// ===================================================================

std::uint16_t checksum_flags_from_name(const std::string& name) {
    if (name == "crc32") {
        return kChecksumFlagCrc32;
    }
    return kChecksumFlagFnv1a;
}

std::string checksum_name_from_flags(const std::uint16_t flags) {
    if ((flags & kChecksumFlagMask) == kChecksumFlagCrc32) {
        return "crc32";
    }
    return "fnv1a";
}

std::string detect_checksum_from_archive(const std::filesystem::path& archive_path) {
    std::ifstream archive(archive_path, std::ios::binary);
    if (!archive) {
        throw std::runtime_error("failed to open archive for checksum detection: " + archive_path.string());
    }

    // Skip magic (4 bytes) + version (2 bytes) to reach flags
    archive.seekg(6);
    if (!archive) {
        throw std::runtime_error("failed to read archive header for checksum detection");
    }

    std::uint16_t flags = 0;
    archive.read(reinterpret_cast<char*>(&flags), static_cast<std::streamsize>(sizeof(flags)));
    if (!archive) {
        throw std::runtime_error("failed to read archive flags for checksum detection");
    }

    return checksum_name_from_flags(flags);
}

}  // namespace backup_system::strategy
