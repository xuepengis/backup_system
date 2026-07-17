#pragma once

#include <memory>
#include <string>
#include <vector>

#include "strategy/ichecksum_engine.hpp"
#include "strategy/istream_processor.hpp"

namespace backup_system::strategy {

/// 根据名称创建压缩编码器实例。
std::shared_ptr<ICompressionCodec> create_compression_codec(const std::string& name);

/// 根据名称创建加密编码器实例。
std::shared_ptr<IEncryptionCodec> create_encryption_codec(const std::string& name);

/// 根据名称创建校验算法实例。
std::shared_ptr<IChecksumEngine> create_checksum_engine(const std::string& name);

/// 返回当前已注册的压缩算法名称列表。
std::vector<std::string> list_compression_codecs();

/// 返回当前已注册的加密算法名称列表。
std::vector<std::string> list_encryption_codecs();

/// 返回当前已注册的校验算法名称列表。
std::vector<std::string> list_checksum_engines();

}  // namespace backup_system::strategy
