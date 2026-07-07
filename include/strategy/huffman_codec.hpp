#pragma once

#include "strategy/istream_processor.hpp"

namespace backup_system::strategy {

class HuffmanCompressionCodec final : public ICompressionCodec {
public:
    std::string name() const override;
    void compress(std::istream& input, std::ostream& output) const override;
    void decompress(std::istream& input, std::ostream& output) const override;
};

}  // namespace backup_system::strategy
