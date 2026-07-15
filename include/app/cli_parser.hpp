#pragma once

#include <string>
#include <vector>

#include "strategy/filter_registry.hpp"
#include "strategy/filter_spec_builder.hpp"

namespace backup_system::app {

struct CliOptions {
    std::string mode;
    std::string source;
    std::string destination;
    std::string compression {"none"};
    std::string encryption {"none"};
    std::string password;
    std::string checksum {"fnv1a"};
    strategy::FilterCliConfig filter_config;
};

class CliParser {
public:
    static CliOptions parse(int argc, char* argv[]);
    static std::string usage(std::string_view program_name);
};

}  // namespace backup_system::app
