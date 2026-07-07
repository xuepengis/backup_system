#include "strategy/huffman_codec.hpp"

#include <array>
#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "utils/huffman_coding.hpp"

namespace backup_system::strategy {

std::string HuffmanCompressionCodec::name() const {
    return "huffman";
}

void HuffmanCompressionCodec::compress(std::istream& input,
                                       std::ostream& output) const {
    std::vector<std::uint8_t> data;
    std::array<char, 64 * 1024> buffer {};

    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto bytes_read = input.gcount();
        for (std::streamsize i = 0; i < bytes_read; ++i) {
            data.push_back(
                static_cast<std::uint8_t>(buffer[static_cast<std::size_t>(i)]));
        }
    }

    if (!input.eof()) {
        throw std::runtime_error(
            "huffman compression failed while reading input");
    }

    utils::HuffmanCoding::encode(data, output);
}

void HuffmanCompressionCodec::decompress(std::istream& input,
                                         std::ostream& output) const {
    auto decoded = utils::HuffmanCoding::decode(input);

    output.write(reinterpret_cast<const char*>(decoded.data()),
                 static_cast<std::streamsize>(decoded.size()));
    if (!output) {
        throw std::runtime_error(
            "huffman decompression failed while writing output");
    }
}

}  // namespace backup_system::strategy
