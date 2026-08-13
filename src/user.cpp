#include <boost/json/object.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "user.h"
#include "connection_pool.h"
#include "http_server.h"
#include "log.h"
#include <exception>
#include <format>
#include <stdexcept>
#include <string>

namespace json = boost::json;
namespace logging = boost::log;
using pqxx::operator""_zv;

Users::Users(ConnectionPool& pool) : pool_(pool){
    if(!LoadUsers()){
        throw std::runtime_error("Users dont load from database");
    }
}

bool Users::LoadUsers(){
    try{
        auto wrapper = pool_.GetConnection();
        pqxx::connection& conn = *wrapper;
        pqxx::read_transaction r(conn);
        constexpr auto query = "SELECT id, username, login, password_hash, token FROM users;"_zv;
        pqxx::result result = r.exec(query);
        users_.reserve(result.size());
        for(const auto& row : result){
            size_t id = row[0].as<size_t>();
            std::string username = row[1].as<std::string>();
            std::string login = row[2].as<std::string>();
            std::string password = row[3].as<std::string>();
            User user {login, password, id, username};
            user.SetToken(row[4].as<std::string>());
            users_.insert({login, user});
        }
    }catch(const std::exception& e){
        json::object obj;
        obj["error"] = e.what();
        obj["message"] = "Database is contains user";
        BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj) << logging::add_value("msg", "User dont register");
        return false;
    }
    return true;
}

User::User(const std::string& login,
    const std::string& pass_hash,
    std::size_t id,
    const std::string& user_name)
    : login_(std::move(login)),
    pass_hash_(std::move(pass_hash)),
    id_(id),
    user_name_(user_name){}

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

const std::string& User::GetUserName()const noexcept {
    return user_name_;
}

std::string Users::GenerateToken(){
    boost::uuids::random_generator gen;
    std::string token = boost::uuids::to_string(gen());
    token.erase(std::remove(token.begin(), token.end(), '-'), token.end());
    return token;
}

std::string Users::RegisterUser(const std::string& login, const std::string& pass_hash) {
    std::scoped_lock lock(mutex_);
    std::string user_name = "user" + std::to_string(users_.size() + 1);
    json::object obj;
    int id = 0;
    std::string token = GenerateToken();
    try{
        constexpr auto query = "INSERT INTO users (username, login, password_hash, token) VALUES ($1, $2, $3, $4) RETURNING id;"_zv;
        auto wrapper = pool_.GetConnection();
        pqxx::connection& conn = *wrapper;
        pqxx::work w(conn);
        auto res = w.exec_params(query, user_name, login, pass_hash, token);
        if(res.empty()){
            throw std::runtime_error("Failed to insert user, no ID returned");
        }
        id = res[0][0].as<int>();
        w.commit();
        obj["code"] = "ok";
        obj["message"] = "User is registered";
        BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj) << logging::add_value("msg", "User is registered: " + login);
    }catch(const std::exception& e){
        obj["error"] = e.what();
        obj["message"] = "Database is contains user";
        BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj) << logging::add_value("msg", "User dont register");
    }
    User user(login, pass_hash, id, user_name);
    user.SetToken(token);
    users_.emplace(login, std::move(user));
    return token;
}

std::size_t Users::GetCounter()const noexcept{
    std::scoped_lock lock(mutex_);
    return counter_;
}

void User::SetToken(const std::string& token) {
    token_ = token;
}

User* Users::FindUserByToken(const std::string& token) {
    std::scoped_lock lock(mutex_);
    for (auto& [login, user] : users_) {
        if (user.GetToken() == token) {
            return &user;
        }
    }
    return nullptr;
}

User* Users::FindUserByUserName(const std::string& user_name) {
    for (auto& [_, user] : users_) {
        if (user.GetUserName() == user_name) {
            return &user;
        }
    }
    return nullptr;
}

User* Users::FindUserByLogin(const std::string& login) {
    std::scoped_lock lock(mutex_);
    auto it = users_.find(login);
    return (it != users_.end()) ? &it->second : nullptr;
}
