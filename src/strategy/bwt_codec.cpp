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
// Suffix array construction — prefix doubling, O(n log² n)
//
// Builds the suffix array for string s: sa[i] is the starting position
// of the i-th smallest suffix.  After at most log₂n rounds the ranks are
// all distinct and the array is fully sorted.
// =========================================================================

std::vector<int> build_sa(const std::vector<std::uint8_t>& s) {
    const int n = static_cast<int>(s.size());
    std::vector<int> sa(n);
    std::vector<int> rank(n);
    std::vector<int> tmp(n);

    // Round 0: sort by the first character only
    std::iota(sa.begin(), sa.end(), 0);
    std::sort(sa.begin(), sa.end(),
              [&](int a, int b) { return s[a] < s[b]; });

    rank[sa[0]] = 0;
    for (int i = 1; i < n; ++i) {
        rank[sa[i]] = rank[sa[i - 1]];
        if (s[sa[i]] != s[sa[i - 1]]) {
            ++rank[sa[i]];
        }
    }

    // Doubling rounds: sort by (rank[i], rank[i+k])
    for (int k = 1; k < n; k *= 2) {
        auto cmp = [&](int a, int b) {
            if (rank[a] != rank[b]) {
                return rank[a] < rank[b];
            }
            int ra = (a + k < n) ? rank[a + k] : -1;
            int rb = (b + k < n) ? rank[b + k] : -1;
            return ra < rb;
        };

        std::sort(sa.begin(), sa.end(), cmp);

        tmp[sa[0]] = 0;
        for (int i = 1; i < n; ++i) {
            tmp[sa[i]] = tmp[sa[i - 1]];
            if (cmp(sa[i - 1], sa[i])) {
                ++tmp[sa[i]];
            }
        }
        rank.swap(tmp);

        // All ranks distinct → fully sorted, early exit
        if (rank[sa[n - 1]] == n - 1) {
            break;
        }
    }

    return sa;
}

// =========================================================================
// BWT forward transform via suffix array
//
// 1. Double the input:  S2 = data + data  (so every rotation is a prefix
//    of some suffix of S2).
// 2. Build the suffix array of S2.
// 3. Walk the SA: for each suffix starting in the first half (sa[i] < n),
//    emit the last column byte  data[(sa[i] + n - 1) % n].
//
// Complexity: O(n log² n) dominated by build_sa.
// =========================================================================

std::vector<std::uint8_t> bwt_forward(const std::vector<std::uint8_t>& data,
                                      int& primary_index) {
    const int n = static_cast<int>(data.size());
    if (n == 0) {
        primary_index = 0;
        return {};
    }

    // Doubled string so rotations are ordinary suffixes
    std::vector<std::uint8_t> s2;
    s2.reserve(static_cast<std::size_t>(2 * n));
    s2.insert(s2.end(), data.begin(), data.end());
    s2.insert(s2.end(), data.begin(), data.end());

    auto sa = build_sa(s2);

    std::vector<std::uint8_t> result;
    result.reserve(static_cast<std::size_t>(n));
    primary_index = 0;

    int bwt_pos = 0;
    for (int i = 0; i < 2 * n; ++i) {
        if (sa[i] < n) {
            if (sa[i] == 0) {
                primary_index = bwt_pos;
            }
            result.push_back(
                s2[static_cast<std::size_t>(sa[i] + n - 1)]);
            ++bwt_pos;
        }
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
