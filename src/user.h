#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <random>
#include <sstream>
#include <iomanip>

class User {
public:
    User() = default;
    User(std::string login, std::string pass_hash, std::size_t id);

    const std::string& GetLogin() const noexcept;
    const std::string& GetPasswordHash() const noexcept;
    std::size_t GetId() const noexcept;
    const std::string& GetToken() const noexcept;
    void SetToken(const std::string& token);
private:
    std::string login_;
    std::string pass_hash_;
    std::size_t id_ = 0;
    std::string token_;
};

class Users {
public:
    std::string RegisterUser(const std::string& login, const std::string& pass_hash);
    User* FindUserByToken(const std::string& token);
    User* FindUserByLogin(const std::string& login);
    std::size_t GetCounter() const noexcept;
private:
    std::string GenerateToken();
    std::unordered_map<std::string, User> users_;   // ключ – логин
    std::size_t counter_ = 0;
    mutable std::mutex mutex_;
};