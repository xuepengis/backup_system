#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include "core/backup_engine.hpp"
#include "strategy/iarchive_strategy.hpp"
#include "strategy/ifile_filter.hpp"
#include "strategy/istream_processor.hpp"

namespace {

struct CliOptions {
    std::string mode;
    std::string source;
    std::string destination;
};

void print_usage(const std::string_view program_name) {
    std::cerr
        << "Usage:\n"
        << "  " << program_name << " --mode backup --src <source_dir> --dest <archive_file>\n"
        << "  " << program_name << " --mode restore --src <archive_file> --dest <restore_dir>\n";
}

CliOptions parse_arguments(int argc, char* argv[]) {
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
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown argument: " + std::string(arg));
        }
    }

    if (options.mode.empty() || options.source.empty() || options.destination.empty()) {
        throw std::invalid_argument("mode, src and dest arguments are required");
    }

    return options;
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        const auto cli_options = parse_arguments(argc, argv);

        auto filter = std::make_shared<backup_system::strategy::PassThroughFileFilter>();
        auto processor = std::make_shared<backup_system::strategy::DirectCopyStreamProcessor>();
        auto archive_strategy = std::make_shared<backup_system::strategy::BinaryArchiveStrategy>();
        backup_system::core::BackupEngine engine(filter, processor, archive_strategy);

        if (cli_options.mode == "backup") {
            engine.backup({
                .source_root = cli_options.source,
                .archive_path = cli_options.destination,
            });
            std::cout << "Backup completed successfully.\n";
            return 0;
        }

        if (cli_options.mode == "restore") {
            engine.restore({
                .archive_path = cli_options.source,
                .restore_root = cli_options.destination,
            });
            std::cout << "Restore completed successfully.\n";
            return 0;
        }

        throw std::invalid_argument("unsupported mode: " + cli_options.mode);
    } catch (const std::exception& ex) {
        print_usage(argc > 0 ? argv[0] : "backup_cli");
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}
