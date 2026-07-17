#pragma once

#include "strategy/istream_processor.hpp"

namespace backup_system::strategy {

/// 基于 Huffman 编码的无损压缩实现。
class HuffmanCompressionCodec final : public ICompressionCodec {
public:
    std::string name() const override;
    void compress(std::istream& input, std::ostream& output) const override;
    void decompress(std::istream& input, std::ostream& output) const override;
};

}  // namespace backup_system::strategy
