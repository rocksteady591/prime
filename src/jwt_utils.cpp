#include "jwt_utils.h"
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <boost/json.hpp>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstring>

const char* JWT_SECRET_KEY = "your-secret-key-change-me";

namespace {

// URL-safe Base64 (без padding)
std::string base64_encode(const std::string& in) {
    static const char* chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789-_"; 
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        out.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    return out;
}

std::string base64_decode(const std::string& in) {
    static const char* chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789-_";
    int T[256];
    std::fill(T, T+256, -1);
    for (int i = 0; i < 64; ++i) T[(unsigned char)chars[i]] = i;

    std::string out;
    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

} // anonymous namespace

std::string GenerateJWT(const std::string& user_id,
                        const std::string& secret,
                        std::chrono::seconds expiry) {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto exp = now + expiry;
    auto iat_sec = duration_cast<seconds>(now.time_since_epoch()).count();
    auto exp_sec = duration_cast<seconds>(exp.time_since_epoch()).count();

    boost::json::object header{{"alg", "HS256"}, {"typ", "JWT"}};
    boost::json::object payload{
        {"sub", user_id},
        {"iat", iat_sec},
        {"exp", exp_sec}
    };
    std::string header_b64 = base64_encode(boost::json::serialize(header));
    std::string payload_b64 = base64_encode(boost::json::serialize(payload));
    std::string data = header_b64 + "." + payload_b64;

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    HMAC(EVP_sha256(),
         secret.c_str(), secret.size(),
         reinterpret_cast<const unsigned char*>(data.c_str()), data.size(),
         digest, &digest_len);
    std::string sig_b64 = base64_encode(std::string(reinterpret_cast<char*>(digest), digest_len));
    return data + "." + sig_b64;
}

bool VerifyJWT(const std::string& token,
               const std::string& secret,
               std::string& out_user_id) {
    size_t pos1 = token.find('.');
    size_t pos2 = token.find('.', pos1 + 1);
    if (pos1 == std::string::npos || pos2 == std::string::npos) return false;

    std::string header_b64 = token.substr(0, pos1);
    std::string payload_b64 = token.substr(pos1 + 1, pos2 - pos1 - 1);
    std::string signature_b64 = token.substr(pos2 + 1);

    // Проверяем подпись
    std::string data = header_b64 + "." + payload_b64;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    HMAC(EVP_sha256(),
         secret.c_str(), secret.size(),
         reinterpret_cast<const unsigned char*>(data.c_str()), data.size(),
         digest, &digest_len);
    std::string expected_sig = base64_encode(std::string(reinterpret_cast<char*>(digest), digest_len));
    if (signature_b64 != expected_sig) return false;

    // Декодируем payload
    std::string payload_json = base64_decode(payload_b64);
    auto payload = boost::json::parse(payload_json).as_object();
    auto exp = payload["exp"].as_int64();
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch()).count();
    if (exp < now) return false; // токен истёк

    out_user_id = payload["sub"].as_string().c_str();
    return true;
}