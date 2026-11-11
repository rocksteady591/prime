#pragma once

#include <string>
#include <chrono>

extern const char* JWT_SECRET_KEY;

// Генерация JWT с использованием HMAC-SHA256
std::string GenerateJWT(const std::string& user_id,
                        const std::string& secret,
                        std::chrono::seconds expiry = std::chrono::hours(24));

// Проверка JWT, при успехе возвращает true и заполняет out_user_id
bool VerifyJWT(const std::string& token,
               const std::string& secret,
               std::string& out_user_id);