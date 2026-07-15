#include <exception>
#include <iostream>
#include <memory>
#include <vector>

#include "app/cli_parser.hpp"
#include "core/backup_engine.hpp"
#include "strategy/codec_registry.hpp"
#include "strategy/filter_registry.hpp"
#include "strategy/iarchive_strategy.hpp"
#include "strategy/ichecksum_engine.hpp"
#include "strategy/ifile_filter.hpp"
#include "strategy/istream_processor.hpp"
#include "utils/logger.hpp"

int main(int argc, char* argv[]) {
    try {
        const auto cli_options = backup_system::app::CliParser::parse(argc, argv);
        const auto filter_specs = backup_system::strategy::FilterSpecBuilder::build_specs(
            cli_options.filter_config,
            cli_options.mode == "backup");

        std::shared_ptr<backup_system::strategy::IFileFilter> filter;
        if (filter_specs.empty()) {
            filter = std::make_shared<backup_system::strategy::PassThroughFileFilter>();
        } else {
            std::vector<std::shared_ptr<backup_system::strategy::IFileFilterRule>> rules;
            rules.reserve(filter_specs.size());
            for (const auto& spec : filter_specs) {
                rules.push_back(backup_system::strategy::create_filter_rule(spec));
            }
            filter = std::make_shared<backup_system::strategy::CompositeFileFilter>(std::move(rules));
        }

        auto compression_codec = backup_system::strategy::create_compression_codec(cli_options.compression);
        auto encryption_codec = backup_system::strategy::create_encryption_codec(cli_options.encryption);
        auto processor = std::make_shared<backup_system::strategy::PipelineStreamProcessor>(
            compression_codec,
            encryption_codec,
            cli_options.password);

        // Determine the checksum engine.  For restore and verify modes the
        // archive header takes precedence — detect_checksum_from_archive()
        // peeks at the header flags so we construct the correct engine up front.
        std::string checksum_name = cli_options.checksum;
        if (cli_options.mode == "restore" || cli_options.mode == "verify") {
            const auto detected = backup_system::strategy::detect_checksum_from_archive(cli_options.source);
            if (cli_options.checksum != "fnv1a" && cli_options.checksum != detected) {
                backup_system::utils::Logger::warning(
                    "archive was created with --checksum " + detected +
                    "; ignoring --checksum " + cli_options.checksum);
            }
            checksum_name = detected;
        }

        auto checksum_engine = backup_system::strategy::create_checksum_engine(checksum_name);
        auto archive_strategy = std::make_shared<backup_system::strategy::BinaryArchiveStrategy>(
            processor->descriptor(), checksum_engine);
        backup_system::core::BackupEngine engine(filter, processor, archive_strategy, checksum_engine);

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

        if (cli_options.mode == "verify") {
            engine.verify({.archive_path = cli_options.source});
            std::cout << "Verification completed successfully.\n";
            return 0;
        }

        throw std::invalid_argument("unsupported mode: " + cli_options.mode);
    } catch (const std::exception& ex) {
        const auto program_name = argc > 0 ? argv[0] : "backup_cli";
        if (std::string_view(ex.what()) != "__print_usage__") {
            std::cerr << backup_system::app::CliParser::usage(program_name);
            std::cerr << "Error: " << ex.what() << '\n';
            return 1;
        }
        std::cout << backup_system::app::CliParser::usage(program_name);
        return 0;
    }
}
