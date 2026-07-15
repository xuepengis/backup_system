#pragma once

#include <cstdint>
#include <filesystem>
#include <istream>
#include <memory>
#include <string>
#include <vector>

namespace backup_system::strategy {

// Archive flags bit 0 encodes the checksum algorithm.
// Other bits are reserved and must remain 0.
constexpr std::uint16_t kChecksumFlagMask = 0x0001;
constexpr std::uint16_t kChecksumFlagFnv1a = 0x0000;
constexpr std::uint16_t kChecksumFlagCrc32 = 0x0001;

// ---------------------------------------------------------------------------
// Abstract checksum engine — strategy for file / stream checksumming.
// Streaming interface (initial_value / update / finalize) supports
// incremental computation inside the archive writer / reader buffers.
// ---------------------------------------------------------------------------
class IChecksumEngine {
public:
    virtual ~IChecksumEngine() = default;

    /// Human-readable algorithm identifier (e.g. "fnv1a", "crc32").
    virtual std::string name() const = 0;

    /// One-shot: compute checksum over an entire input stream.
    virtual std::uint64_t compute_stream(std::istream& input) const = 0;

    /// One-shot: compute checksum over an on-disk file.
    virtual std::uint64_t compute_file(const std::filesystem::path& path) const = 0;

    /// Streaming interface for incremental computation.
    virtual std::uint64_t initial_value() const = 0;
    virtual void update(std::uint64_t& state,
                        const char* data,
                        std::size_t size) const = 0;
    virtual std::uint64_t finalize(std::uint64_t state) const = 0;
};

// ---------------------------------------------------------------------------
// Concrete engines
// ---------------------------------------------------------------------------

class Fnv1aChecksumEngine final : public IChecksumEngine {
public:
    std::string name() const override;
    std::uint64_t compute_stream(std::istream& input) const override;
    std::uint64_t compute_file(const std::filesystem::path& path) const override;
    std::uint64_t initial_value() const override;
    void update(std::uint64_t& state, const char* data, std::size_t size) const override;
    std::uint64_t finalize(std::uint64_t state) const override;
};

class Crc32ChecksumEngine final : public IChecksumEngine {
public:
    std::string name() const override;
    std::uint64_t compute_stream(std::istream& input) const override;
    std::uint64_t compute_file(const std::filesystem::path& path) const override;
    std::uint64_t initial_value() const override;
    void update(std::uint64_t& state, const char* data, std::size_t size) const override;
    std::uint64_t finalize(std::uint64_t state) const override;
};

// ---------------------------------------------------------------------------
// Factory + registry (follows codec_registry pattern)
// ---------------------------------------------------------------------------

std::shared_ptr<IChecksumEngine> create_checksum_engine(const std::string& name);
std::vector<std::string> list_checksum_engines();

// ---------------------------------------------------------------------------
// Flag conversion helpers
// ---------------------------------------------------------------------------

/// Map a checksum engine name to the corresponding archive flags bit pattern.
std::uint16_t checksum_flags_from_name(const std::string& name);

/// Extract the checksum engine name from raw archive header flags.
std::string checksum_name_from_flags(std::uint16_t flags);

/// Peek at an existing archive file header and determine which checksum
/// algorithm was used.  Returns the engine name (e.g. "fnv1a" or "crc32").
std::string detect_checksum_from_archive(const std::filesystem::path& archive_path);

}  // namespace backup_system::strategy
