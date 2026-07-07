#include "strategy/lz77_codec.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace backup_system::strategy {

namespace {

// ---------------------------------------------------------------------------
// LZ77 format constants
// ---------------------------------------------------------------------------
constexpr int kWindowSize = 4096;         // max look-back distance (12 bits)
constexpr int kMinMatch = 3;              // shortest match we encode
constexpr int kMaxMatch = 258;            // longest match (3 + 255)
constexpr int kHashBits = 15;             // hash table size = 32768
constexpr int kHashSize = 1 << kHashBits;
constexpr int kHashMask = kHashSize - 1;
constexpr int kNoMatch = -1;

// ---------------------------------------------------------------------------
// Hash three bytes into a table index.
// ---------------------------------------------------------------------------
inline int hash3(std::uint8_t a, std::uint8_t b, std::uint8_t c) {
    return ((static_cast<int>(a) << 10) ^
            (static_cast<int>(b) << 5) ^
             static_cast<int>(c)) &
           kHashMask;
}

// ---------------------------------------------------------------------------
// BitWriter: accumulates bits MSB-first into a byte buffer, flushes to output.
// ---------------------------------------------------------------------------
class BitWriter {
public:
    explicit BitWriter(std::ostream& output) : output_(output) {}

    void write_bits(std::uint64_t value, int count) {
        for (int i = count - 1; i >= 0; --i) {
            buffer_ = static_cast<std::uint8_t>(
                (buffer_ << 1) | static_cast<int>((value >> i) & 1));
            ++bit_count_;
            if (bit_count_ == 8) {
                output_.put(static_cast<char>(buffer_));
                buffer_ = 0;
                bit_count_ = 0;
            }
        }
    }

    void flush() {
        if (bit_count_ > 0) {
            buffer_ = static_cast<std::uint8_t>(buffer_ << (8 - bit_count_));
            output_.put(static_cast<char>(buffer_));
            buffer_ = 0;
            bit_count_ = 0;
        }
    }

private:
    std::ostream& output_;
    std::uint8_t buffer_ = 0;
    int bit_count_ = 0;
};

// ---------------------------------------------------------------------------
// BitReader: reads single bits and multi-bit values from input.
// ---------------------------------------------------------------------------
class BitReader {
public:
    explicit BitReader(std::istream& input) : input_(input) {}

    int read_bit() {
        if (bit_count_ == 0) {
            int byte = input_.get();
            if (byte == std::char_traits<char>::eof()) {
                return -1;
            }
            buffer_ = static_cast<std::uint8_t>(byte);
            bit_count_ = 8;
        }
        --bit_count_;
        return (buffer_ >> bit_count_) & 1;
    }

    std::uint64_t read_bits(int count) {
        std::uint64_t value = 0;
        for (int i = 0; i < count; ++i) {
            int bit = read_bit();
            if (bit < 0) {
                throw std::runtime_error(
                    "lz77 decompression failed: unexpected end of bitstream");
            }
            value = (value << 1) | static_cast<std::uint64_t>(bit);
        }
        return value;
    }

private:
    std::istream& input_;
    std::uint8_t buffer_ = 0;
    int bit_count_ = 0;
};

// ---------------------------------------------------------------------------
// Find the longest match at position `pos` looking back up to `window`.
// Returns (offset, length).  offset=0 means no match found.
// ---------------------------------------------------------------------------
std::pair<int, int> find_match(const std::vector<std::uint8_t>& data,
                                const std::vector<int>& hash_table,
                                int pos) {
    if (pos + kMinMatch > static_cast<int>(data.size())) {
        return {0, 0};
    }

    int best_offset = 0;
    int best_length = 0;

    int candidate = hash_table[hash3(data[pos], data[pos + 1], data[pos + 2])];
    if (candidate != kNoMatch && (pos - candidate) <= kWindowSize && candidate < pos) {
        // Extend the match
        int len = 0;
        int max_extend = std::min(kMaxMatch,
                                  static_cast<int>(data.size()) - pos);
        while (len < max_extend &&
               data[candidate + len] == data[pos + len]) {
            ++len;
        }
        if (len >= kMinMatch) {
            best_offset = pos - candidate;
            best_length = len;
        }
    }

    return {best_offset, best_length};
}

}  // namespace

// ===================================================================
// Lz77CompressionCodec
// ===================================================================

std::string Lz77CompressionCodec::name() const {
    return "lz77";
}

void Lz77CompressionCodec::compress(std::istream& input,
                                    std::ostream& output) const {
    // Read entire input
    std::vector<std::uint8_t> data;
    std::array<char, 64 * 1024> buffer {};

    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto bytes = input.gcount();
        for (std::streamsize i = 0; i < bytes; ++i) {
            data.push_back(static_cast<std::uint8_t>(
                buffer[static_cast<std::size_t>(i)]));
        }
    }
    if (!input.eof()) {
        throw std::runtime_error("lz77 compression failed while reading input");
    }

    const auto original_size = static_cast<std::uint64_t>(data.size());

    // Write header: 8 bytes original size
    output.write(reinterpret_cast<const char*>(&original_size),
                 static_cast<std::streamsize>(sizeof(original_size)));
    if (!output) {
        throw std::runtime_error("lz77 compression failed while writing header");
    }

    if (original_size == 0) {
        return;
    }

    // Hash table: maps 3-byte hash → most recent position
    std::vector<int> hash_table(kHashSize, kNoMatch);

    BitWriter writer(output);
    int pos = 0;

    while (pos < static_cast<int>(data.size())) {
        // Update hash for current position (before processing)
        if (pos + 2 < static_cast<int>(data.size())) {
            int h = hash3(data[pos], data[pos + 1], data[pos + 2]);
            hash_table[h] = pos;
        }

        auto [offset, length] = find_match(data, hash_table, pos);

        if (length >= kMinMatch) {
            // Emit match token: flag=1 + 12-bit offset + 8-bit length
            writer.write_bits(1, 1);
            writer.write_bits(static_cast<std::uint64_t>(offset - 1), 12);
            writer.write_bits(static_cast<std::uint64_t>(length - kMinMatch), 8);

            // Update hash for all intermediate positions covered by the match
            for (int i = 1; i < length && (pos + i + 2) < static_cast<int>(data.size()); ++i) {
                int h = hash3(data[pos + i], data[pos + i + 1], data[pos + i + 2]);
                hash_table[h] = pos + i;
            }

            pos += length;
        } else {
            // Emit literal token: flag=0 + 8-bit byte
            writer.write_bits(0, 1);
            writer.write_bits(data[pos], 8);
            ++pos;
        }
    }

    writer.flush();
    if (!output) {
        throw std::runtime_error(
            "lz77 compression failed while writing encoded data");
    }
}

void Lz77CompressionCodec::decompress(std::istream& input,
                                      std::ostream& output) const {
    // Read header: original size
    std::uint64_t original_size = 0;
    input.read(reinterpret_cast<char*>(&original_size),
               static_cast<std::streamsize>(sizeof(original_size)));
    if (!input) {
        throw std::runtime_error(
            "lz77 decompression failed while reading header");
    }

    if (original_size == 0) {
        return;
    }

    // Output buffer for back-references (sliding window)
    std::vector<std::uint8_t> decoded;
    decoded.reserve(static_cast<std::size_t>(original_size));

    BitReader reader(input);

    while (decoded.size() < original_size) {
        int flag = reader.read_bit();
        if (flag < 0) {
            throw std::runtime_error(
                "lz77 decompression failed: unexpected end of bitstream");
        }

        if (flag == 0) {
            // Literal
            auto byte = static_cast<std::uint8_t>(reader.read_bits(8));
            decoded.push_back(byte);
        } else {
            // Match reference
            auto offset_val = static_cast<int>(reader.read_bits(12));
            auto length_val = static_cast<int>(reader.read_bits(8));
            int offset = offset_val + 1;
            int length = length_val + kMinMatch;

            if (offset > static_cast<int>(decoded.size())) {
                throw std::runtime_error(
                    "lz77 decompression failed: invalid back-reference offset");
            }

            int ref_pos = static_cast<int>(decoded.size()) - offset;
            for (int i = 0; i < length; ++i) {
                decoded.push_back(decoded[ref_pos + i]);
            }
        }
    }

    // Write decoded data
    output.write(reinterpret_cast<const char*>(decoded.data()),
                 static_cast<std::streamsize>(decoded.size()));
    if (!output) {
        throw std::runtime_error(
            "lz77 decompression failed while writing output");
    }
}

}  // namespace backup_system::strategy
