#pragma once
#include <pqxx/pqxx>
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
    Users(pqxx::connection& sql);
    std::string RegisterUser(const std::string& login, const std::string& pass_hash);
    User* FindUserByToken(const std::string& token);
    User* FindUserByLogin(const std::string& login);
    User* FindUserByUserName(const std::string& user_name);
    std::size_t GetCounter() const noexcept;
private:
    pqxx::connection& sql_;
    std::string GenerateToken();
    std::unordered_map<std::string, User> users_;   // ключ – логин
    std::size_t counter_ = 0;
    mutable std::mutex mutex_;
};