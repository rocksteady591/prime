#include "user.h"
#include <random>
#include <format>

User::User(std::string login, std::string pass_hash, std::size_t id)
    : login_(std::move(login)),
    pass_hash_(std::move(pass_hash)),
    id_(id)
{
}

const std::string& User::GetLogin() const noexcept {
    return login_;
}

const std::string& User::GetPasswordHash() const noexcept {
    return pass_hash_;
}

std::size_t User::GetId() const noexcept {
    return id_;
}

const std::string& User::GetToken()const noexcept{
    return token_;
}

std::string Users::GenerateToken(){
    std::random_device rd;
    std::mt19937_64::result_type seed = rd();
    std::mt19937_64 gen(seed);
    std::uniform_int_distribution<uint64_t> dist;

    uint64_t part1 = dist(gen);
    uint64_t part2 = dist(gen);

    return std::format("{:016x}{:016x}", part1, part2);
}

std::string Users::RegisterUser(const std::string& login, const std::string& pass_hash) {
    std::lock_guard lock(mutex_);
    // Проверяем, нет ли уже такого логина
    if (users_.contains(login)) {
        return "";  // или исключение
    }
    User user(login, pass_hash, counter_++);
    std::string token = GenerateToken();
    user.SetToken(token);
    users_.emplace(login, std::move(user));
    return token;
}

std::size_t Users::GetCounter()const noexcept{
    std::lock_guard lock(mutex_);
    return counter_;
}

void User::SetToken(const std::string& token) {
    token_ = token;
}

User* Users::FindUserByToken(const std::string& token) {
    std::lock_guard lock(mutex_);
    for (auto& [login, user] : users_) {
        if (user.GetToken() == token) {
            return &user;
        }
    }
    return nullptr;
}

User* Users::FindUserByLogin(const std::string& login) {
    std::lock_guard lock(mutex_);
    auto it = users_.find(login);
    return (it != users_.end()) ? &it->second : nullptr;
}