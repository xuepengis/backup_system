#include "strategy/aes_gcm_codec.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <openssl/evp.h>
#include <openssl/rand.h>

namespace backup_system::strategy {

namespace {

// ---------------------------------------------------------------------------
// Algorithm constants
// ---------------------------------------------------------------------------
constexpr int kKeySize = 32;
constexpr int kIvSize = 12;
constexpr int kTagSize = 16;
constexpr int kSaltSize = 16;
constexpr int kPbkdf2Iterations = 100000;

// ---------------------------------------------------------------------------
// RAII wrapper for EVP_CIPHER_CTX
// ---------------------------------------------------------------------------
struct EvpCipherCtxDeleter {
    void operator()(EVP_CIPHER_CTX* ctx) const {
        if (ctx != nullptr) {
            EVP_CIPHER_CTX_free(ctx);
        }
    }
};
using EvpCipherCtxPtr = std::unique_ptr<EVP_CIPHER_CTX, EvpCipherCtxDeleter>;

// ---------------------------------------------------------------------------
// Generate cryptographically secure random bytes
// ---------------------------------------------------------------------------
void generate_random_bytes(std::uint8_t* buffer, const std::size_t length) {
    if (RAND_bytes(buffer, static_cast<int>(length)) != 1) {
        throw std::runtime_error(
            "aes-gcm encryption failed: unable to generate random bytes");
    }
}

// ---------------------------------------------------------------------------
// Derive a 256-bit key from password + salt using PBKDF2-HMAC-SHA256
// ---------------------------------------------------------------------------
std::array<std::uint8_t, kKeySize> derive_key(const std::string_view password,
                                               const std::uint8_t* salt) {
    std::array<std::uint8_t, kKeySize> key {};
    if (PKCS5_PBKDF2_HMAC(password.data(), static_cast<int>(password.size()),
                          salt, kSaltSize,
                          kPbkdf2Iterations,
                          EVP_sha256(),
                          kKeySize, key.data()) != 1) {
        throw std::runtime_error(
            "aes-gcm encryption failed: key derivation failed");
    }
    return key;
}

// ---------------------------------------------------------------------------
// Read entire input stream into a byte vector
// ---------------------------------------------------------------------------
std::vector<std::uint8_t> read_all(std::istream& input) {
    std::vector<std::uint8_t> data;
    std::array<char, 64 * 1024> buffer {};

    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto bytes_read = input.gcount();
        for (std::streamsize i = 0; i < bytes_read; ++i) {
            data.push_back(
                static_cast<std::uint8_t>(buffer[static_cast<std::size_t>(i)]));
        }
    }

    if (!input.eof()) {
        throw std::runtime_error(
            "aes-gcm encryption failed while reading input");
    }
    return data;
}

// ---------------------------------------------------------------------------
// Create and initialise an EVP cipher context for encryption
// ---------------------------------------------------------------------------
EvpCipherCtxPtr create_encrypt_ctx(const std::uint8_t* key,
                                    const std::uint8_t* iv) {
    EvpCipherCtxPtr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        throw std::runtime_error(
            "aes-gcm encryption failed: unable to allocate cipher context");
    }

    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(),
                           nullptr, nullptr, nullptr) != 1) {
        throw std::runtime_error(
            "aes-gcm encryption failed: cipher initialisation failed");
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN,
                            kIvSize, nullptr) != 1) {
        throw std::runtime_error(
            "aes-gcm encryption failed: IV length configuration failed");
    }

    if (EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, key, iv) != 1) {
        throw std::runtime_error(
            "aes-gcm encryption failed: key/IV setup failed");
    }

    return ctx;
}

// ---------------------------------------------------------------------------
// Create and initialise an EVP cipher context for decryption
// ---------------------------------------------------------------------------
EvpCipherCtxPtr create_decrypt_ctx(const std::uint8_t* key,
                                    const std::uint8_t* iv,
                                    const std::uint8_t* tag) {
    EvpCipherCtxPtr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        throw std::runtime_error(
            "aes-gcm decryption failed: unable to allocate cipher context");
    }

    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(),
                           nullptr, nullptr, nullptr) != 1) {
        throw std::runtime_error(
            "aes-gcm decryption failed: cipher initialisation failed");
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN,
                            kIvSize, nullptr) != 1) {
        throw std::runtime_error(
            "aes-gcm decryption failed: IV length configuration failed");
    }

    if (EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, key, iv) != 1) {
        throw std::runtime_error(
            "aes-gcm decryption failed: key/IV setup failed");
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG,
                            kTagSize, const_cast<std::uint8_t*>(tag)) != 1) {
        throw std::runtime_error(
            "aes-gcm decryption failed: tag configuration failed");
    }

    return ctx;
}

}  // namespace

// ===================================================================
// AesGcmEncryptionCodec
//
// Format: [16-byte PBKDF2 salt][12-byte IV][ciphertext][16-byte GCM tag]
//
// Key derivation: PBKDF2-HMAC-SHA256, 100 000 iterations
// Cipher: AES-256-GCM via OpenSSL EVP
// ===================================================================

std::string AesGcmEncryptionCodec::name() const {
    return "aes-256-gcm";
}

bool AesGcmEncryptionCodec::requires_password() const {
    return true;
}

void AesGcmEncryptionCodec::encrypt(std::istream& input,
                                     std::ostream& output,
                                     const std::string_view password) const {
    if (password.empty()) {
        throw std::invalid_argument("aes-256-gcm requires a non-empty password");
    }

    // Generate random salt and IV
    std::array<std::uint8_t, kSaltSize> salt {};
    std::array<std::uint8_t, kIvSize> iv {};
    generate_random_bytes(salt.data(), salt.size());
    generate_random_bytes(iv.data(), iv.size());

    // Derive encryption key
    const auto key = derive_key(password, salt.data());

    // Write salt and IV to output
    output.write(reinterpret_cast<const char*>(salt.data()),
                 static_cast<std::streamsize>(salt.size()));
    if (!output) {
        throw std::runtime_error(
            "aes-gcm encryption failed while writing salt");
    }
    output.write(reinterpret_cast<const char*>(iv.data()),
                 static_cast<std::streamsize>(iv.size()));
    if (!output) {
        throw std::runtime_error(
            "aes-gcm encryption failed while writing IV");
    }

    // Read all plaintext
    const auto plaintext = read_all(input);

    // Initialise encryption context
    auto ctx = create_encrypt_ctx(key.data(), iv.data());

    // Encrypt
    std::vector<std::uint8_t> ciphertext(plaintext.size() + 16);
    int out_len = 0;

    if (EVP_EncryptUpdate(ctx.get(), ciphertext.data(), &out_len,
                          plaintext.data(),
                          static_cast<int>(plaintext.size())) != 1) {
        throw std::runtime_error(
            "aes-gcm encryption failed during EVP_EncryptUpdate");
    }
    int total_len = out_len;

    if (EVP_EncryptFinal_ex(ctx.get(),
                            ciphertext.data() + total_len,
                            &out_len) != 1) {
        throw std::runtime_error(
            "aes-gcm encryption failed during EVP_EncryptFinal");
    }
    total_len += out_len;

    // Write ciphertext
    output.write(reinterpret_cast<const char*>(ciphertext.data()),
                 static_cast<std::streamsize>(total_len));
    if (!output) {
        throw std::runtime_error(
            "aes-gcm encryption failed while writing ciphertext");
    }

    // Retrieve and write GCM authentication tag
    std::array<std::uint8_t, kTagSize> tag {};
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG,
                            kTagSize, tag.data()) != 1) {
        throw std::runtime_error(
            "aes-gcm encryption failed: unable to retrieve authentication tag");
    }

    output.write(reinterpret_cast<const char*>(tag.data()),
                 static_cast<std::streamsize>(tag.size()));
    if (!output) {
        throw std::runtime_error(
            "aes-gcm encryption failed while writing authentication tag");
    }
}

void AesGcmEncryptionCodec::decrypt(std::istream& input,
                                     std::ostream& output,
                                     const std::string_view password) const {
    if (password.empty()) {
        throw std::invalid_argument("aes-256-gcm requires a non-empty password");
    }

    // Read salt
    std::array<std::uint8_t, kSaltSize> salt {};
    input.read(reinterpret_cast<char*>(salt.data()),
               static_cast<std::streamsize>(salt.size()));
    if (!input) {
        throw std::runtime_error(
            "aes-gcm decryption failed: unable to read salt (archive may be truncated)");
    }

    // Read IV
    std::array<std::uint8_t, kIvSize> iv {};
    input.read(reinterpret_cast<char*>(iv.data()),
               static_cast<std::streamsize>(iv.size()));
    if (!input) {
        throw std::runtime_error(
            "aes-gcm decryption failed: unable to read IV (archive may be truncated)");
    }

    // Read ciphertext + tag
    const auto encrypted_data = read_all(input);

    if (encrypted_data.size() < static_cast<std::size_t>(kTagSize)) {
        throw std::runtime_error(
            "aes-gcm decryption failed: encrypted data is too short "
            "(missing authentication tag)");
    }

    const auto ciphertext_size = encrypted_data.size() -
                                 static_cast<std::size_t>(kTagSize);
    const auto* const ciphertext = encrypted_data.data();
    const auto* const tag = encrypted_data.data() + ciphertext_size;

    // Derive key
    const auto key = derive_key(password, salt.data());

    // Initialise decryption context with expected tag
    auto ctx = create_decrypt_ctx(key.data(), iv.data(), tag);

    // Decrypt
    std::vector<std::uint8_t> plaintext(ciphertext_size);
    int out_len = 0;

    if (EVP_DecryptUpdate(ctx.get(), plaintext.data(), &out_len,
                          ciphertext,
                          static_cast<int>(ciphertext_size)) != 1) {
        throw std::runtime_error(
            "aes-gcm decryption failed during EVP_DecryptUpdate");
    }
    int total_len = out_len;

    const int final_result = EVP_DecryptFinal_ex(
        ctx.get(), plaintext.data() + total_len, &out_len);
    if (final_result != 1) {
        throw std::runtime_error(
            "aes-gcm decryption failed: authentication tag mismatch "
            "(wrong password or corrupted data)");
    }
    total_len += out_len;

    // Write plaintext
    output.write(reinterpret_cast<const char*>(plaintext.data()),
                 static_cast<std::streamsize>(total_len));
    if (!output) {
        throw std::runtime_error(
            "aes-gcm decryption failed while writing output");
    }
}

}  // namespace backup_system::strategy
