#include "utils/huffman_coding.hpp"

#include <array>
#include <cstdint>
#include <istream>
#include <ostream>
#include <queue>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace backup_system::utils {

namespace {

// ---------------------------------------------------------------------------
// Huffman tree node
// ---------------------------------------------------------------------------
struct HuffNode {
    int left = -1;
    int right = -1;
    std::uint64_t freq = 0;
    bool is_leaf = false;
    std::uint8_t symbol = 0;
};

// ---------------------------------------------------------------------------
// Build Huffman tree from frequency table.
// ---------------------------------------------------------------------------
int build_huffman_tree(const std::array<std::uint64_t, 256>& freqs,
                       std::vector<HuffNode>& nodes) {
    nodes.clear();

    int active_count = 0;
    for (int i = 0; i < 256; ++i) {
        if (freqs[i] > 0) {
            nodes.push_back({-1, -1, freqs[i], true,
                             static_cast<std::uint8_t>(i)});
            ++active_count;
        }
    }

    if (active_count == 0) {
        return -1;
    }

    if (active_count == 1) {
        return 0;
    }

    auto cmp = [&](int a, int b) { return nodes[a].freq > nodes[b].freq; };
    std::priority_queue<int, std::vector<int>, decltype(cmp)> pq(cmp);
    for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
        pq.push(i);
    }

    while (pq.size() > 1) {
        int left  = pq.top(); pq.pop();
        int right = pq.top(); pq.pop();
        nodes.push_back({left, right, nodes[left].freq + nodes[right].freq,
                         false, 0});
        pq.push(static_cast<int>(nodes.size()) - 1);
    }

    return pq.top();
}

// ---------------------------------------------------------------------------
// DFS traversal to generate code table.
// ---------------------------------------------------------------------------
void generate_huffman_codes(
    const std::vector<HuffNode>& nodes,
    int node_idx,
    std::uint64_t code,
    int depth,
    std::array<std::pair<std::uint64_t, int>, 256>& codes) {
    const auto& node = nodes[node_idx];
    if (node.is_leaf) {
        codes[node.symbol] = {code, depth};
        return;
    }
    generate_huffman_codes(nodes, node.left, code << 1, depth + 1, codes);
    generate_huffman_codes(nodes, node.right, (code << 1) | 1, depth + 1,
                           codes);
}

// ---------------------------------------------------------------------------
// BitWriter / BitReader
// ---------------------------------------------------------------------------
class BitWriter {
public:
    explicit BitWriter(std::ostream& output) : output_(output) {}

    void write_bits(std::uint64_t value, int num_bits) {
        for (int i = num_bits - 1; i >= 0; --i) {
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

private:
    std::istream& input_;
    std::uint8_t buffer_ = 0;
    int bit_count_ = 0;
};

}  // namespace

// ===================================================================
// Public API
// ===================================================================

void HuffmanCoding::encode(const std::vector<std::uint8_t>& data,
                           std::ostream& output) {
    // Count frequencies
    std::array<std::uint64_t, 256> freqs {};
    for (auto byte : data) {
        ++freqs[byte];
    }

    const auto original_size = static_cast<std::uint64_t>(data.size());

    // Write header: 8-byte size + 256×8-byte frequencies
    output.write(reinterpret_cast<const char*>(&original_size),
                 static_cast<std::streamsize>(sizeof(original_size)));
    for (int i = 0; i < 256; ++i) {
        output.write(reinterpret_cast<const char*>(&freqs[i]),
                     static_cast<std::streamsize>(sizeof(std::uint64_t)));
    }
    if (!output) {
        throw std::runtime_error("huffman coding: failed to write header");
    }

    if (original_size == 0) {
        return;
    }

    // Build tree and generate codes
    std::vector<HuffNode> nodes;
    int root = build_huffman_tree(freqs, nodes);

    std::array<std::pair<std::uint64_t, int>, 256> codes {};
    if (nodes.size() == 1) {
        codes[data[0]] = {0, 0};
    } else {
        generate_huffman_codes(nodes, root, 0, 0, codes);
    }

    // Encode
    BitWriter writer(output);
    for (auto byte : data) {
        const auto& [code, length] = codes[byte];
        writer.write_bits(code, length);
    }
    writer.flush();

    if (!output) {
        throw std::runtime_error("huffman coding: failed to write encoded data");
    }
}

std::vector<std::uint8_t> HuffmanCoding::decode(std::istream& input) {
    // Read header
    std::uint64_t original_size = 0;
    input.read(reinterpret_cast<char*>(&original_size),
               static_cast<std::streamsize>(sizeof(original_size)));
    if (!input) {
        throw std::runtime_error("huffman coding: failed to read header");
    }

    std::array<std::uint64_t, 256> freqs {};
    for (int i = 0; i < 256; ++i) {
        input.read(reinterpret_cast<char*>(&freqs[i]),
                   static_cast<std::streamsize>(sizeof(std::uint64_t)));
    }
    if (!input) {
        throw std::runtime_error("huffman coding: failed to read frequency table");
    }

    if (original_size == 0) {
        return {};
    }

    // Detect single-symbol case
    int active_count = 0;
    std::uint8_t single_symbol = 0;
    for (int i = 0; i < 256; ++i) {
        if (freqs[i] > 0) {
            ++active_count;
            single_symbol = static_cast<std::uint8_t>(i);
        }
    }

    std::vector<std::uint8_t> result;
    result.reserve(static_cast<std::size_t>(original_size));

    if (active_count == 1) {
        result.assign(static_cast<std::size_t>(original_size), single_symbol);
        return result;
    }

    // Rebuild tree
    std::vector<HuffNode> nodes;
    int root = build_huffman_tree(freqs, nodes);

    // Decode bitstream by walking the tree
    BitReader reader(input);
    std::uint64_t decoded = 0;
    int node = root;

    while (decoded < original_size) {
        int bit = reader.read_bit();
        if (bit < 0) {
            throw std::runtime_error(
                "huffman coding: unexpected end of bitstream");
        }
        node = (bit == 0) ? nodes[node].left : nodes[node].right;
        if (node < 0) {
            throw std::runtime_error("huffman coding: invalid tree traversal");
        }
        if (nodes[node].is_leaf) {
            result.push_back(nodes[node].symbol);
            ++decoded;
            node = root;
        }
    }

    return result;
}

}  // namespace backup_system::utils
