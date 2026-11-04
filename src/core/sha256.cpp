#include "core/sha256.hpp"

#include <openssl/sha.h>
#include <stdexcept>

namespace bsl {
std::string sha256(std::string_view text) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    // A default-constructed string_view may have a null data pointer.
    const auto* bytes = reinterpret_cast<const unsigned char*>(text.empty() ? "" : text.data());
    if (SHA256(bytes, text.size(), digest) == nullptr) {
        throw std::runtime_error("OpenSSL SHA-256 failed");
    }
    constexpr char hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(SHA256_DIGEST_LENGTH * 2);
    for (const auto byte : digest) {
        result.push_back(hex[byte >> 4]);
        result.push_back(hex[byte & 15]);
    }
    return result;
}
}
