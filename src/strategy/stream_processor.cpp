#include "strategy/istream_processor.hpp"

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include <unistd.h>

namespace backup_system::strategy {

namespace {

void copy_stream(std::istream& input, std::ostream& output) {
    std::array<char, 64 * 1024> buffer {};

    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto bytes_read = input.gcount();
        if (bytes_read > 0) {
            output.write(buffer.data(), bytes_read);
        }
    }

    if (!input.eof()) {
        throw std::runtime_error("stream copy failed while reading input");
    }
    if (!output) {
        throw std::runtime_error("stream copy failed while writing output");
    }
}

class ScopedTempFile final {
public:
    ScopedTempFile() {
        std::array<char, 64> path_template {};
        const auto written =
            std::snprintf(path_template.data(), path_template.size(), "/tmp/backup_system_codec_XXXXXX");
        if (written <= 0 || static_cast<std::size_t>(written) >= path_template.size()) {
            throw std::runtime_error("failed to prepare temporary file template");
        }

        const int fd = ::mkstemp(path_template.data());
        if (fd < 0) {
            throw std::runtime_error("failed to create temporary file");
        }
        ::close(fd);
        path_ = path_template.data();
    }

    ~ScopedTempFile() {
        if (!path_.empty()) {
            std::filesystem::remove(path_);
        }
    }

    ScopedTempFile(const ScopedTempFile&) = delete;
    ScopedTempFile& operator=(const ScopedTempFile&) = delete;

    const std::filesystem::path& path() const {
        return path_;
    }

private:
    std::filesystem::path path_;
};

}  // namespace

PipelineStreamProcessor::PipelineStreamProcessor(std::shared_ptr<ICompressionCodec> compression_codec,
                                                 std::shared_ptr<IEncryptionCodec> encryption_codec,
                                                 std::string password)
    : compression_codec_(std::move(compression_codec)),
      encryption_codec_(std::move(encryption_codec)),
      password_(std::move(password)) {
    if (!compression_codec_) {
        throw std::invalid_argument("compression codec must not be null");
    }
    if (!encryption_codec_) {
        throw std::invalid_argument("encryption codec must not be null");
    }
    if (encryption_codec_->requires_password() && password_.empty()) {
        throw std::invalid_argument("selected encryption codec requires a password");
    }
}

PayloadCodecDescriptor PipelineStreamProcessor::descriptor() const {
    return {
        .compression_name = compression_codec_->name(),
        .encryption_name = encryption_codec_->name(),
    };
}

void PipelineStreamProcessor::backup(std::istream& input,
                                     std::ostream& output,
                                     const std::filesystem::path& source_path) const {
    (void)source_path;

    if (compression_codec_->name() == "none" && encryption_codec_->name() == "none") {
        copy_stream(input, output);
        return;
    }
    if (compression_codec_->name() == "none") {
        encryption_codec_->encrypt(input, output, password_);
        return;
    }
    if (encryption_codec_->name() == "none") {
        compression_codec_->compress(input, output);
        return;
    }

    ScopedTempFile temp_file;
    {
        std::ofstream temp_output(temp_file.path(), std::ios::binary | std::ios::trunc);
        if (!temp_output) {
            throw std::runtime_error("failed to open temporary file for compression");
        }
        compression_codec_->compress(input, temp_output);
    }
    std::ifstream temp_input(temp_file.path(), std::ios::binary);
    if (!temp_input) {
        throw std::runtime_error("failed to reopen temporary file for encryption");
    }
    encryption_codec_->encrypt(temp_input, output, password_);
}

void PipelineStreamProcessor::restore(std::istream& input,
                                      std::ostream& output,
                                      const std::filesystem::path& archived_path) const {
    (void)archived_path;

    if (compression_codec_->name() == "none" && encryption_codec_->name() == "none") {
        copy_stream(input, output);
        return;
    }
    if (compression_codec_->name() == "none") {
        encryption_codec_->decrypt(input, output, password_);
        return;
    }
    if (encryption_codec_->name() == "none") {
        compression_codec_->decompress(input, output);
        return;
    }

    ScopedTempFile temp_file;
    {
        std::ofstream temp_output(temp_file.path(), std::ios::binary | std::ios::trunc);
        if (!temp_output) {
            throw std::runtime_error("failed to open temporary file for decryption");
        }
        encryption_codec_->decrypt(input, temp_output, password_);
    }
    std::ifstream temp_input(temp_file.path(), std::ios::binary);
    if (!temp_input) {
        throw std::runtime_error("failed to reopen temporary file for decompression");
    }
    compression_codec_->decompress(temp_input, output);
}

}  // namespace backup_system::strategy
