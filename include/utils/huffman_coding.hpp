#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <vector>

namespace backup_system::utils {

class HuffmanCoding {
public:
    /// Encode raw data using Huffman coding.
    /// Writes: 8-byte original size + 256×8-byte frequency table + bitstream.
    static void encode(const std::vector<std::uint8_t>& data,
                       std::ostream& output);

    /// Decode Huffman-encoded stream back to raw data.
    /// Reads the header written by encode().
    static std::vector<std::uint8_t> decode(std::istream& input);
};

}  // namespace backup_system::utils
