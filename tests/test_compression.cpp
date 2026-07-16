#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "strategy/codec_registry.hpp"

using backup_system::strategy::create_compression_codec;
using backup_system::strategy::ICompressionCodec;

namespace {

// Build a string with every byte value 0x00..0xFF
std::string all_bytes_string() {
    std::string result;
    result.reserve(256);
    for (int b = 0; b < 256; ++b) {
        result.push_back(static_cast<char>(b));
    }
    return result;
}

// Build a string of repeated text
std::string repeated_text(std::size_t count, const std::string& pattern) {
    std::string result;
    result.reserve(pattern.size() * count);
    for (std::size_t i = 0; i < count; ++i) {
        result.append(pattern);
    }
    return result;
}

// Round-trip: compress then decompress, return the result
std::string roundtrip_compress(ICompressionCodec& codec,
                                const std::string& input) {
    std::istringstream in(input, std::ios::binary);
    std::ostringstream compressed(std::ios::binary);
    codec.compress(in, compressed);

    std::istringstream comp_in(compressed.str(), std::ios::binary);
    std::ostringstream decompressed(std::ios::binary);
    codec.decompress(comp_in, decompressed);
    return decompressed.str();
}

}  // namespace

// ===========================================================================
// NoCompression (none)
// ===========================================================================

TEST_CASE("NoCompression round-trip preserves data", "[compression]") {
    auto codec = create_compression_codec("none");

    SECTION("name") {
        REQUIRE(codec->name() == "none");
    }

    SECTION("empty input") {
        REQUIRE(roundtrip_compress(*codec, "").empty());
    }

    SECTION("single byte") {
        REQUIRE(roundtrip_compress(*codec, "\x42") == "\x42");
    }

    SECTION("all byte values") {
        const auto original = all_bytes_string();
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }

    SECTION("text data") {
        const std::string original = "Hello, World! This is a test.";
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }

    SECTION("large data (64KB)") {
        const auto original = repeated_text(1024, "ABCDEFGH");
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }
}

// ===========================================================================
// RLE Compression
// ===========================================================================

TEST_CASE("RLE round-trip with varied data", "[compression]") {
    auto codec = create_compression_codec("rle");
    REQUIRE(codec->name() == "rle");

    SECTION("empty input") {
        REQUIRE(roundtrip_compress(*codec, "").empty());
    }

    SECTION("single byte") {
        REQUIRE(roundtrip_compress(*codec, "X") == "X");
    }

    SECTION("mixed bytes") {
        const std::string original = "AABBBCCCC";
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }

    SECTION("all unique bytes (no runs)") {
        const auto original = all_bytes_string();
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }

    SECTION("all byte values repeated") {
        // 256 unique values, each repeated 3 times
        std::string original;
        for (int b = 0; b < 256; ++b) {
            original.push_back(static_cast<char>(b));
            original.push_back(static_cast<char>(b));
            original.push_back(static_cast<char>(b));
        }
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }
}

TEST_CASE("RLE round-trip with repeated bytes", "[compression]") {
    auto codec = create_compression_codec("rle");

    SECTION("256 A's (2 runs: 255+1)") {
        const std::string original(256, 'A');

        // Compress
        std::istringstream in(original, std::ios::binary);
        std::ostringstream compressed(std::ios::binary);
        codec->compress(in, compressed);

        const auto compressed_str = compressed.str();
        // 255×A + 1×A = 4 bytes (count=255, 'A', count=1, 'A')
        REQUIRE(compressed_str.size() == 4);

        // Decompress
        std::istringstream comp_in(compressed_str, std::ios::binary);
        std::ostringstream decompressed(std::ios::binary);
        codec->decompress(comp_in, decompressed);

        REQUIRE(decompressed.str() == original);
    }

    SECTION("500 X's") {
        const std::string original(500, 'X');
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }

    SECTION("single repeated byte") {
        const std::string original(42, 'Z');
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }
}

TEST_CASE("RLE empty input produces minimal output", "[compression]") {
    auto codec = create_compression_codec("rle");

    // Compress empty
    std::istringstream empty_in("", std::ios::binary);
    std::ostringstream compressed(std::ios::binary);
    codec->compress(empty_in, compressed);
    REQUIRE(compressed.str().empty());

    // Decompress empty
    std::istringstream empty_comp_in("", std::ios::binary);
    std::ostringstream decompressed(std::ios::binary);
    codec->decompress(empty_comp_in, decompressed);
    REQUIRE(decompressed.str().empty());
}

// ===========================================================================
// Huffman Compression
// ===========================================================================

TEST_CASE("Huffman round-trip with text data", "[compression]") {
    auto codec = create_compression_codec("huffman");
    REQUIRE(codec->name() == "huffman");

    SECTION("simple text") {
        const std::string original = "hello hello hello world";
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }

    SECTION("repeated pattern") {
        const auto original = repeated_text(200, "The quick brown fox ");
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }

    SECTION("all byte values") {
        const auto original = all_bytes_string();
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }

    SECTION("large text (100KB)") {
        const auto original = repeated_text(10000, "ABCDEFGHIJ");
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }
}

TEST_CASE("Huffman round-trip empty input", "[compression]") {
    auto codec = create_compression_codec("huffman");
    REQUIRE(roundtrip_compress(*codec, "").empty());
}

TEST_CASE("Huffman round-trip single byte input", "[compression]") {
    auto codec = create_compression_codec("huffman");

    // Single byte — special case in HuffmanCoding (no bitstream, only header)
    const std::string original = "\xAB";
    const auto result = roundtrip_compress(*codec, original);
    REQUIRE(result == original);
}

// ===========================================================================
// LZ77 Compression
// ===========================================================================

TEST_CASE("LZ77 round-trip with repeated patterns", "[compression]") {
    auto codec = create_compression_codec("lz77");
    REQUIRE(codec->name() == "lz77");

    SECTION("repeating 10-byte string 100 times") {
        const auto original = repeated_text(100, "0123456789");
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }

    SECTION("all same byte") {
        const std::string original(4096, 'X');
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }

    SECTION("literals only (random-like)") {
        const auto original = all_bytes_string();
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }

    SECTION("short data (less than min match)") {
        REQUIRE(roundtrip_compress(*codec, "AB") == "AB");
    }
}

TEST_CASE("LZ77 round-trip empty input", "[compression]") {
    auto codec = create_compression_codec("lz77");
    REQUIRE(roundtrip_compress(*codec, "").empty());
}

TEST_CASE("LZ77 round-trip data larger than window", "[compression]") {
    auto codec = create_compression_codec("lz77");
    // LZ77 window is 4096 bytes — data larger than window will contain
    // matches beyond window range, which become literals
    const auto original = repeated_text(500, "ABCDEFGH");  // 4000 bytes
    // Add extra text beyond window
    const auto full = original + std::string(200, 'Z');
    REQUIRE(roundtrip_compress(*codec, full) == full);
}

// ===========================================================================
// BWT Compression
// ===========================================================================

TEST_CASE("BWT round-trip with text data", "[compression]") {
    auto codec = create_compression_codec("bwt");
    REQUIRE(codec->name() == "bwt");

    SECTION("repeated text") {
        const auto original = repeated_text(50, "banana");
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }

    SECTION("english-like text") {
        const auto original = repeated_text(100, "The quick brown fox jumps over the lazy dog. ");
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }

    SECTION("all byte values") {
        const auto original = all_bytes_string();
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }

    SECTION("short text") {
        const std::string original = "Hello, BWT!";
        REQUIRE(roundtrip_compress(*codec, original) == original);
    }
}

TEST_CASE("BWT empty input round-trip", "[compression]") {
    auto codec = create_compression_codec("bwt");
    REQUIRE(roundtrip_compress(*codec, "").empty());
}

TEST_CASE("BWT achieves compression on repetitive text", "[compression]") {
    auto codec = create_compression_codec("bwt");
    // Highly repetitive text should compress well
    const auto original = repeated_text(500, "banana ");

    std::istringstream in(original, std::ios::binary);
    std::ostringstream compressed(std::ios::binary);
    codec->compress(in, compressed);

    // BWT + MTF + Huffman should achieve some compression on repetitive data
    const auto compressed_size = compressed.str().size();
    REQUIRE(compressed_size < original.size());

    // Verify round-trip
    std::istringstream comp_in(compressed.str(), std::ios::binary);
    std::ostringstream decompressed(std::ios::binary);
    codec->decompress(comp_in, decompressed);
    REQUIRE(decompressed.str() == original);
}
