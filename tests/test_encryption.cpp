#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>
#include <stdexcept>

#include "strategy/codec_registry.hpp"

using backup_system::strategy::create_encryption_codec;
using backup_system::strategy::IEncryptionCodec;

namespace {

// Round-trip: encrypt then decrypt, return the result
std::string roundtrip_encrypt(IEncryptionCodec& codec,
                               const std::string& input,
                               const std::string& password) {
    std::istringstream in(input, std::ios::binary);
    std::ostringstream encrypted(std::ios::binary);
    codec.encrypt(in, encrypted, password);

    std::istringstream enc_in(encrypted.str(), std::ios::binary);
    std::ostringstream decrypted(std::ios::binary);
    codec.decrypt(enc_in, decrypted, password);
    return decrypted.str();
}

// Encrypt only (for checking ciphertext != plaintext)
std::string encrypt_only(IEncryptionCodec& codec,
                          const std::string& input,
                          const std::string& password) {
    std::istringstream in(input, std::ios::binary);
    std::ostringstream encrypted(std::ios::binary);
    codec.encrypt(in, encrypted, password);
    return encrypted.str();
}

}  // namespace

// ===========================================================================
// NoEncryption (none)
// ===========================================================================

TEST_CASE("NoEncryption round-trip is identity", "[encryption]") {
    auto codec = create_encryption_codec("none");

    SECTION("name and password requirement") {
        REQUIRE(codec->name() == "none");
        REQUIRE_FALSE(codec->requires_password());
    }

    SECTION("empty input") {
        REQUIRE(roundtrip_encrypt(*codec, "", "").empty());
    }

    SECTION("data with any password (ignored)") {
        const std::string original = "secret data";
        REQUIRE(roundtrip_encrypt(*codec, original, "ignored") == original);
    }

    SECTION("encrypt is identity (data unchanged)") {
        const std::string original = "hello world";
        const auto encrypted = encrypt_only(*codec, original, "ignored");
        REQUIRE(encrypted == original);
    }
}

// ===========================================================================
// XOR-stream Encryption
// ===========================================================================

TEST_CASE("XOR-stream encrypt+decrypt round-trip", "[encryption]") {
    auto codec = create_encryption_codec("xor-stream");

    SECTION("name and password requirement") {
        REQUIRE(codec->name() == "xor-stream");
        REQUIRE(codec->requires_password());
    }

    SECTION("empty input") {
        REQUIRE(roundtrip_encrypt(*codec, "", "password").empty());
    }

    SECTION("single byte") {
        REQUIRE(roundtrip_encrypt(*codec, "X", "secret") == "X");
    }

    SECTION("text data") {
        const std::string original = "Hello, World! This is a secret message.";
        REQUIRE(roundtrip_encrypt(*codec, original, "mypassword") == original);
    }

    SECTION("large binary data") {
        std::string original(100000, '\0');
        for (std::size_t i = 0; i < original.size(); ++i) {
            original[i] = static_cast<char>(i % 256);
        }
        REQUIRE(roundtrip_encrypt(*codec, original, "large-key") == original);
    }

    SECTION("encrypt does not equal plaintext") {
        const std::string original = "sensitive data here";
        const auto encrypted = encrypt_only(*codec, original, "key123");
        REQUIRE(encrypted != original);
    }

    SECTION("different passwords produce different ciphertexts") {
        const std::string original = "test data";
        const auto enc1 = encrypt_only(*codec, original, "alice");
        const auto enc2 = encrypt_only(*codec, original, "bob");
        REQUIRE(enc1 != enc2);
    }
}

TEST_CASE("XOR-stream empty password throws", "[encryption]") {
    auto codec = create_encryption_codec("xor-stream");

    std::istringstream in("data", std::ios::binary);
    std::ostringstream out(std::ios::binary);

    REQUIRE_THROWS_AS(codec->encrypt(in, out, ""), std::invalid_argument);
}

// ===========================================================================
// AES-256-GCM Encryption
// ===========================================================================

TEST_CASE("AES-256-GCM encrypt+decrypt round-trip", "[encryption]") {
    auto codec = create_encryption_codec("aes-256-gcm");

    SECTION("name and password requirement") {
        REQUIRE(codec->name() == "aes-256-gcm");
        REQUIRE(codec->requires_password());
    }

    SECTION("empty input") {
        REQUIRE(roundtrip_encrypt(*codec, "", "strong-password").empty());
    }

    SECTION("single byte") {
        REQUIRE(roundtrip_encrypt(*codec, "X", "pass123") == "X");
    }

    SECTION("text data") {
        const std::string original = "The quick brown fox jumps over the lazy dog.";
        REQUIRE(roundtrip_encrypt(*codec, original, "secret123") == original);
    }

    SECTION("binary data with all byte values") {
        std::string original(256, '\0');
        for (int i = 0; i < 256; ++i) {
            original[i] = static_cast<char>(i);
        }
        REQUIRE(roundtrip_encrypt(*codec, original, "binary-key") == original);
    }

    SECTION("large data (100KB)") {
        std::string original(100000, '\0');
        for (std::size_t i = 0; i < original.size(); ++i) {
            original[i] = static_cast<char>((i * 7 + 3) % 256);
        }
        REQUIRE(roundtrip_encrypt(*codec, original, "big-file-key") == original);
    }

    SECTION("random salt/IV makes ciphertext different each time") {
        const std::string original = "consistent input";
        const auto enc1 = encrypt_only(*codec, original, "key");
        const auto enc2 = encrypt_only(*codec, original, "key");
        // Same password, same plaintext → different ciphertext due to random salt
        REQUIRE(enc1 != enc2);
    }
}

TEST_CASE("AES-256-GCM wrong password detected", "[encryption]") {
    auto codec = create_encryption_codec("aes-256-gcm");

    const std::string original = "classified information";
    const std::string correct_password = "correct-horse-battery-staple";
    const std::string wrong_password = "wrong-password-here";

    // Encrypt with correct password
    std::istringstream in(original, std::ios::binary);
    std::ostringstream encrypted(std::ios::binary);
    codec->encrypt(in, encrypted, correct_password);

    // Try to decrypt with wrong password — should fail (tag mismatch)
    std::istringstream enc_in(encrypted.str(), std::ios::binary);
    std::ostringstream decrypted(std::ios::binary);

    REQUIRE_THROWS_AS(codec->decrypt(enc_in, decrypted, wrong_password),
                      std::runtime_error);
}

// ===========================================================================
// ChaCha20-Poly1305 Encryption
// ===========================================================================

TEST_CASE("ChaCha20-Poly1305 encrypt+decrypt round-trip", "[encryption]") {
    auto codec = create_encryption_codec("chacha20-poly1305");

    SECTION("name and password requirement") {
        REQUIRE(codec->name() == "chacha20-poly1305");
        REQUIRE(codec->requires_password());
    }

    SECTION("empty input") {
        REQUIRE(roundtrip_encrypt(*codec, "", "password").empty());
    }

    SECTION("single byte") {
        REQUIRE(roundtrip_encrypt(*codec, "X", "testkey") == "X");
    }

    SECTION("text data") {
        const std::string original = "ChaCha20 is a modern stream cipher.";
        REQUIRE(roundtrip_encrypt(*codec, original, "secure-pw") == original);
    }

    SECTION("binary data") {
        std::string original(256, '\0');
        for (int i = 0; i < 256; ++i) {
            original[i] = static_cast<char>(i);
        }
        REQUIRE(roundtrip_encrypt(*codec, original, "binary-key") == original);
    }

    SECTION("random salt/nonce makes ciphertext different") {
        const std::string original = "reproducible plaintext";
        const auto enc1 = encrypt_only(*codec, original, "key");
        const auto enc2 = encrypt_only(*codec, original, "key");
        REQUIRE(enc1 != enc2);
    }
}

TEST_CASE("ChaCha20-Poly1305 wrong password detected", "[encryption]") {
    auto codec = create_encryption_codec("chacha20-poly1305");

    const std::string original = "top secret data";

    // Encrypt
    std::istringstream in(original, std::ios::binary);
    std::ostringstream encrypted(std::ios::binary);
    codec->encrypt(in, encrypted, "right-password");

    // Decrypt with wrong password
    std::istringstream enc_in(encrypted.str(), std::ios::binary);
    std::ostringstream decrypted(std::ios::binary);

    REQUIRE_THROWS_AS(codec->decrypt(enc_in, decrypted, "wrong-password"),
                      std::runtime_error);
}
