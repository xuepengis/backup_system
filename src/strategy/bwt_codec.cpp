#include "strategy/bwt_codec.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <istream>
#include <numeric>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "utils/huffman_coding.hpp"

namespace backup_system::strategy {

namespace {

// =========================================================================
// Move-to-Front transform
// =========================================================================

std::vector<std::uint8_t> mtf_encode(const std::vector<std::uint8_t>& data) {
    std::array<std::uint8_t, 256> list;
    for (int i = 0; i < 256; ++i) list[i] = static_cast<std::uint8_t>(i);

    std::vector<std::uint8_t> result;
    result.reserve(data.size());

    for (std::uint8_t byte : data) {
        int pos = 0;
        while (list[pos] != byte) ++pos;

        result.push_back(static_cast<std::uint8_t>(pos));

        for (int i = pos; i > 0; --i) list[i] = list[i - 1];
        list[0] = byte;
    }
    return result;
}

std::vector<std::uint8_t> mtf_decode(const std::vector<std::uint8_t>& data) {
    std::array<std::uint8_t, 256> list;
    for (int i = 0; i < 256; ++i) list[i] = static_cast<std::uint8_t>(i);

    std::vector<std::uint8_t> result;
    result.reserve(data.size());

    for (std::uint8_t pos : data) {
        std::uint8_t byte = list[pos];
        result.push_back(byte);

        for (int i = pos; i > 0; --i) list[i] = list[i - 1];
        list[0] = byte;
    }
    return result;
}

// =========================================================================
// BWT forward transform
// =========================================================================

std::vector<std::uint8_t> bwt_forward(const std::vector<std::uint8_t>& data,
                                      int& primary_index) {
    const int n = static_cast<int>(data.size());
    if (n == 0) {
        primary_index = 0;
        return {};
    }

    std::vector<int> rotations(n);
    std::iota(rotations.begin(), rotations.end(), 0);

    std::sort(rotations.begin(), rotations.end(),
              [&](int a, int b) {
                  for (int i = 0; i < n; ++i) {
                      std::uint8_t ba = data[(a + i) % n];
                      std::uint8_t bb = data[(b + i) % n];
                      if (ba != bb) return ba < bb;
                  }
                  return false;
              });

    primary_index = 0;
    std::vector<std::uint8_t> result;
    result.reserve(n);

    for (int i = 0; i < n; ++i) {
        int r = rotations[i];
        result.push_back(data[(r - 1 + n) % n]);
        if (r == 0) primary_index = i;
    }
    return result;
}

// =========================================================================
// BWT inverse transform (table-based reconstruction)
// =========================================================================

std::vector<std::uint8_t> bwt_inverse(const std::vector<std::uint8_t>& last_col,
                                      int primary_index) {
    const int n = static_cast<int>(last_col.size());
    if (n == 0) return {};

    std::vector<std::pair<std::uint8_t, int>> table;
    table.reserve(n);
    for (int i = 0; i < n; ++i) {
        table.emplace_back(last_col[i], i);
    }
    std::sort(table.begin(), table.end());

    std::vector<std::uint8_t> result;
    result.reserve(n);
    int row = primary_index;
    for (int i = 0; i < n; ++i) {
        result.push_back(table[row].first);
        row = table[row].second;
    }
    return result;
}

}  // namespace

// ===================================================================
// BwtCompressionCodec
//
// Pipeline: data -> BWT -> MTF -> HuffmanCoding -> output
// ===================================================================

std::string BwtCompressionCodec::name() const {
    return "bwt";
}

void BwtCompressionCodec::compress(std::istream& input,
                                   std::ostream& output) const {
    std::vector<std::uint8_t> data;
    std::array<char, 64 * 1024> buffer {};

    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto bytes = input.gcount();
        for (std::streamsize i = 0; i < bytes; ++i) {
            data.push_back(
                static_cast<std::uint8_t>(buffer[static_cast<std::size_t>(i)]));
        }
    }
    if (!input.eof()) {
        throw std::runtime_error("bwt compression failed while reading input");
    }

    const auto original_size = static_cast<std::uint64_t>(data.size());

    output.write(reinterpret_cast<const char*>(&original_size),
                 static_cast<std::streamsize>(sizeof(original_size)));
    if (!output) {
        throw std::runtime_error("bwt compression failed while writing header");
    }

    if (original_size == 0) return;

    // Stage 1: BWT
    int primary_index = 0;
    auto bwt_out = bwt_forward(data, primary_index);

    output.write(reinterpret_cast<const char*>(&primary_index),
                 static_cast<std::streamsize>(sizeof(primary_index)));
    if (!output) {
        throw std::runtime_error(
            "bwt compression failed while writing primary index");
    }

    // Stage 2: MTF
    auto mtf_out = mtf_encode(bwt_out);

    // Stage 3: Huffman encode (shared utility)
    utils::HuffmanCoding::encode(mtf_out, output);
}

void BwtCompressionCodec::decompress(std::istream& input,
                                     std::ostream& output) const {
    std::uint64_t original_size = 0;
    input.read(reinterpret_cast<char*>(&original_size),
               static_cast<std::streamsize>(sizeof(original_size)));
    if (!input) {
        throw std::runtime_error("bwt decompression failed while reading header");
    }

    if (original_size == 0) return;

    int primary_index = 0;
    input.read(reinterpret_cast<char*>(&primary_index),
               static_cast<std::streamsize>(sizeof(primary_index)));
    if (!input) {
        throw std::runtime_error(
            "bwt decompression failed while reading primary index");
    }

    // Stage 1: Huffman decode (shared utility)
    auto mtf_out = utils::HuffmanCoding::decode(input);

    if (mtf_out.size() != original_size) {
        throw std::runtime_error(
            "bwt decompression: decoded size does not match expected");
    }

    // Stage 2: MTF decode → BWT output
    auto bwt_out = mtf_decode(mtf_out);

    // Stage 3: BWT inverse → original data
    auto original = bwt_inverse(bwt_out, primary_index);

    if (original.size() != original_size) {
        throw std::runtime_error(
            "bwt decompression: restored size does not match expected");
    }

    output.write(reinterpret_cast<const char*>(original.data()),
                 static_cast<std::streamsize>(original.size()));
    if (!output) {
        throw std::runtime_error(
            "bwt decompression failed while writing output");
    }
}

}  // namespace backup_system::strategy
