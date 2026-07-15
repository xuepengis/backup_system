#pragma once

#include <memory>
#include <string>
#include <vector>

#include "strategy/ichecksum_engine.hpp"
#include "strategy/istream_processor.hpp"

namespace backup_system::strategy {

std::shared_ptr<ICompressionCodec> create_compression_codec(const std::string& name);
std::shared_ptr<IEncryptionCodec> create_encryption_codec(const std::string& name);
std::shared_ptr<IChecksumEngine> create_checksum_engine(const std::string& name);
std::vector<std::string> list_compression_codecs();
std::vector<std::string> list_encryption_codecs();
std::vector<std::string> list_checksum_engines();

}  // namespace backup_system::strategy
