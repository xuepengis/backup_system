#pragma once

#include <string>
#include <vector>

#include "strategy/filter_registry.hpp"
#include "strategy/filter_spec_builder.hpp"

namespace backup_system::app {

/// 命令行解析结果，作为应用层与核心流程之间的参数载体。
struct CliOptions {
    /// 运行模式：`backup` / `restore` / `verify`。
    std::string mode;
    /// 输入路径，备份模式下为源目录，其余模式下为归档文件。
    std::string source;
    /// 输出路径，备份模式下为归档文件，恢复模式下为目标目录。
    std::string destination;
    /// 压缩算法标识，默认不压缩。
    std::string compression {"none"};
    /// 加密算法标识，默认不加密。
    std::string encryption {"none"};
    /// 加密口令，仅在启用加密时生效。
    std::string password;
    /// 校验算法标识，用于归档内容校验。
    std::string checksum {"fnv1a"};
    /// 文件过滤相关参数。
    strategy::FilterCliConfig filter_config;
};

/// 负责解析命令行参数并生成统一的应用配置。
class CliParser {
public:
    /// 解析命令行参数，参数缺失或不合法时抛出异常。
    static CliOptions parse(int argc, char* argv[]);

    /// 生成命令行帮助文本。
    static std::string usage(std::string_view program_name);
};

}  // namespace backup_system::app
