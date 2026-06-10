#include "user.h"
#include <random>
#include <format>

User::User(std::string login, std::string pass, std::size_t id)
: login_(std::move(login)), 
hash_pass_(std::move(pass)), 
id_(id),
token_(std::move(GenerateToken())){}

const std::string& User::GetToken()const noexcept{
    return token_;
}

std::string User::GenerateToken(){
    std::random_device rd;
    std::mt19937_64 gen(rd);
    std::uniform_int_distribution<uint64_t> dist;

    uint64_t part1 = dist(gen);
    uint64_t part2 = dist(gen);

    return std::format("{:016x}{:016x}", part1, part2);
}

void Users::CreateUser(std::string login, std::string password){
    std::lock_guard lock(mutex_);
    User user(std::move(login), std::move(password), GetCounter());
    users_.insert({user.GetToken(), user});
    ++counter_;
}

std::size_t Users::GetCounter()const noexcept{
    std::lock_guard lock(mutex_);
    return counter_;
}

User* Users::FindUser(const std::string& token){
    std::lock_guard lock(mutex_);
    if(users_.contains(token)){
        return &users_.at(token);
    }
    return nullptr;
}