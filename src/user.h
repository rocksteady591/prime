#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <random>
#include <sstream>
#include <iomanip>
#include "connection_pool.h"

class User {
public:
    User() = default;
    User(const std::string& login,
        const std::string& pass_hash,
        std::size_t id,
        const std::string& user_name);

    const std::string& GetLogin() const noexcept;
    const std::string& GetPasswordHash() const noexcept;
    std::size_t GetId() const noexcept;
    const std::string& GetToken() const noexcept;
    void SetToken(const std::string& token);
    const std::string& GetUserName()const noexcept;
private:
    std::string login_;
    std::string pass_hash_;
    std::size_t id_ = 0;
    std::string user_name_;
    std::string token_;
};

class Users {
public:
    Users(ConnectionPool& pool);
    std::string RegisterUser(const std::string& login, const std::string& pass_hash);
    User* FindUserByToken(const std::string& token);
    User* FindUserByLogin(const std::string& login);
    User* FindUserByUserName(const std::string& user_name);
    std::size_t GetCounter() const noexcept;
private:
    ConnectionPool& pool_;
    std::string GenerateToken();
    bool LoadUsers();
    std::unordered_map<std::string, User> users_;   // ключ – логин
    std::size_t counter_ = 0;
    mutable std::mutex mutex_;
};
