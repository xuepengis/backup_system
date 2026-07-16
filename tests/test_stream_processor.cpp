#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "strategy/codec_registry.hpp"
#include "strategy/istream_processor.hpp"

using backup_system::strategy::create_compression_codec;
using backup_system::strategy::create_encryption_codec;
using backup_system::strategy::PipelineStreamProcessor;

// ===========================================================================
// Constructor validation
// ===========================================================================

TEST_CASE("PipelineStreamProcessor rejects null codecs", "[stream]") {
    auto valid_compression = create_compression_codec("none");
    auto valid_encryption = create_encryption_codec("none");
    std::shared_ptr<backup_system::strategy::ICompressionCodec> null_compression;
    std::shared_ptr<backup_system::strategy::IEncryptionCodec> null_encryption;

    SECTION("null compression codec throws") {
        REQUIRE_THROWS_AS(
            PipelineStreamProcessor(null_compression, valid_encryption, ""),
            std::invalid_argument);
    }

    SECTION("null encryption codec throws") {
        REQUIRE_THROWS_AS(
            PipelineStreamProcessor(valid_compression, null_encryption, ""),
            std::invalid_argument);
    }
}

TEST_CASE("PipelineStreamProcessor requires password for encryption", "[stream]") {
    auto compression = create_compression_codec("none");
    auto encryption = create_encryption_codec("xor-stream");

    SECTION("empty password throws when encryption requires it") {
        REQUIRE(encryption->requires_password());
        REQUIRE_THROWS_AS(
            PipelineStreamProcessor(compression, encryption, ""),
            std::invalid_argument);
    }

    SECTION("password provided works") {
        REQUIRE_NOTHROW(
            PipelineStreamProcessor(compression, encryption, "secret"));
    }
}

// ===========================================================================
// Chained pipeline round-trip
// ===========================================================================

TEST_CASE("PipelineStreamProcessor backup+restore round-trip", "[stream]") {
    SECTION("huffman compression only (no encryption)") {
        auto compression = create_compression_codec("huffman");
        auto encryption = create_encryption_codec("none");

        PipelineStreamProcessor processor(compression, encryption, "");

        auto desc = processor.descriptor();
        REQUIRE(desc.compression_name == "huffman");
        REQUIRE(desc.encryption_name == "none");

        const std::string original = "Hello, this is pipeline test data! "
                                     "Repeated to have some size. "
                                     "Hello, this is pipeline test data!";

        // Backup: compress
        std::istringstream in(original, std::ios::binary);
        std::ostringstream compressed(std::ios::binary);
        processor.backup(in, compressed, "dummy_path");

        // Restore: decompress
        std::istringstream comp_in(compressed.str(), std::ios::binary);
        std::ostringstream decompressed(std::ios::binary);
        processor.restore(comp_in, decompressed, "dummy_path");

        REQUIRE(decompressed.str() == original);
    }

    SECTION("xor-stream encryption only (no compression)") {
        auto compression = create_compression_codec("none");
        auto encryption = create_encryption_codec("xor-stream");

        PipelineStreamProcessor processor(compression, encryption, "test-key");

        const std::string original = "Secret message for pipeline test.";

        std::istringstream in(original, std::ios::binary);
        std::ostringstream encrypted(std::ios::binary);
        processor.backup(in, encrypted, "dummy");

        std::istringstream enc_in(encrypted.str(), std::ios::binary);
        std::ostringstream decrypted(std::ios::binary);
        processor.restore(enc_in, decrypted, "dummy");

        REQUIRE(decrypted.str() == original);
    }

    SECTION("huffman + xor-stream chained") {
        auto compression = create_compression_codec("huffman");
        auto encryption = create_encryption_codec("xor-stream");

        PipelineStreamProcessor processor(compression, encryption, "chain-key");

        const std::string original =
            std::string(2000, 'A') + std::string(2000, 'B') +
            std::string(2000, 'C');

        std::istringstream in(original, std::ios::binary);
        std::ostringstream processed(std::ios::binary);
        processor.backup(in, processed, "file");

        std::istringstream proc_in(processed.str(), std::ios::binary);
        std::ostringstream restored(std::ios::binary);
        processor.restore(proc_in, restored, "file");

        REQUIRE(restored.str() == original);
    }

    SECTION("no-op pipeline (both none)") {
        auto compression = create_compression_codec("none");
        auto encryption = create_encryption_codec("none");

        PipelineStreamProcessor processor(compression, encryption, "");

        const std::string original = "Pass-through data.";

        std::istringstream in(original, std::ios::binary);
        std::ostringstream out(std::ios::binary);
        processor.backup(in, out, "path");

        std::istringstream proc_in(out.str(), std::ios::binary);
        std::ostringstream restored(std::ios::binary);
        processor.restore(proc_in, restored, "path");

        REQUIRE(restored.str() == original);
    }
}
