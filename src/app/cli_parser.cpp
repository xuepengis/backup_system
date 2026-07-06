#include "app/cli_parser.hpp"

#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "strategy/codec_registry.hpp"

namespace backup_system::app {

namespace {

std::string join_options(const std::vector<std::string>& values) {
    std::ostringstream builder;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            builder << '|';
        }
        builder << values[index];
    }
    return builder.str();
}

void validate_options(const CliOptions& options) {
    if (options.mode.empty() || options.source.empty() || options.destination.empty()) {
        throw std::invalid_argument("mode, src and dest arguments are required");
    }
    if (options.mode != "backup" && options.mode != "restore") {
        throw std::invalid_argument("mode must be either backup or restore");
    }
    if (options.encryption == "none" && !options.password.empty()) {
        throw std::invalid_argument("password was provided but encryption is disabled");
    }
    if (options.encryption != "none" && options.password.empty()) {
        throw std::invalid_argument("selected encryption codec requires --password");
    }

    (void)strategy::FilterSpecBuilder::build_specs(
        options.filter_config,
        options.mode == "backup");
}

}  // namespace

CliOptions CliParser::parse(int argc, char* argv[]) {
    CliOptions options;

    for (int index = 1; index < argc; ++index) {
        const std::string_view arg {argv[index]};

        auto require_value = [&](const std::string_view name) -> std::string {
            if (index + 1 >= argc) {
                throw std::invalid_argument("missing value for argument: " + std::string(name));
            }
            ++index;
            return argv[index];
        };

        if (arg == "--mode") {
            options.mode = require_value(arg);
        } else if (arg == "--src") {
            options.source = require_value(arg);
        } else if (arg == "--dest") {
            options.destination = require_value(arg);
        } else if (arg == "--compression") {
            options.compression = require_value(arg);
        } else if (arg == "--encryption") {
            options.encryption = require_value(arg);
        } else if (arg == "--password") {
            options.password = require_value(arg);
        } else if (arg == "--include-path") {
            options.filter_config.include_paths.push_back(require_value(arg));
        } else if (arg == "--include-name") {
            options.filter_config.include_names.push_back(require_value(arg));
        } else if (arg == "--min-size") {
            options.filter_config.min_size_text = require_value(arg);
        } else if (arg == "--max-size") {
            options.filter_config.max_size_text = require_value(arg);
        } else if (arg == "--modified-after") {
            options.filter_config.modified_after_text = require_value(arg);
        } else if (arg == "--modified-before") {
            options.filter_config.modified_before_text = require_value(arg);
        } else if (arg == "--help" || arg == "-h") {
            throw std::invalid_argument("__print_usage__");
        } else {
            throw std::invalid_argument("unknown argument: " + std::string(arg));
        }
    }

    validate_options(options);
    return options;
}

std::string CliParser::usage(const std::string_view program_name) {
    const auto compression_options = join_options(strategy::list_compression_codecs());
    const auto encryption_options = join_options(strategy::list_encryption_codecs());

    std::ostringstream output;
    output
        << "Usage:\n"
        << "  " << program_name
        << " --mode backup --src <source_dir> --dest <archive_file> [--compression " << compression_options
        << "] [--encryption " << encryption_options << "] [--password <secret>]\n"
        << "    [--include-path <glob>] [--include-name <glob>] [--min-size <bytes>] [--max-size <bytes>]\n"
        << "    [--modified-after <YYYY-MM-DDTHH:MM:SS>] [--modified-before <YYYY-MM-DDTHH:MM:SS>]\n"
        << "  " << program_name
        << " --mode restore --src <archive_file> --dest <restore_dir> [--compression " << compression_options
        << "] [--encryption " << encryption_options << "] [--password <secret>]\n";
    return output.str();
}

}  // namespace backup_system::app
