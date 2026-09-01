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
#include <sodium.h>
#include "jwt_utils.h"

namespace json = boost::json;
namespace logging = boost::log;
using pqxx::operator""_zv;

static constexpr size_t HASH_LEN = crypto_pwhash_STRBYTES; // 128 байт
static constexpr size_t SALT_LEN = crypto_pwhash_SALTBYTES; // 16 байт


static std::string HashPassword(const std::string& password) {
    char hash[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str(hash, password.c_str(), password.length(),
                          crypto_pwhash_OPSLIMIT_INTERACTIVE,
                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        throw std::runtime_error("Password hashing failed");
    }
    return std::string(hash);
}

bool Users::VerifyPassword(const std::string& password, const std::string& stored_hash) {
    return crypto_pwhash_str_verify(stored_hash.c_str(), password.c_str(), password.length()) == 0;
}

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
        pqxx::result result = r.exec_params(query);
        users_by_login_.reserve(result.size());
        users_by_id_.reserve(result.size());
        for (const auto& row : result){
            size_t id = row[0].as<size_t>();
            std::string username = row[1].as<std::string>();
            std::string login = row[2].as<std::string>();
            std::string password = row[3].as<std::string>();
            User user(login, password, id, username);
            user.SetToken(row[4].as<std::string>());
            users_by_login_.emplace(login, user);
            users_by_id_.emplace(id, user);
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


std::string Users::RegisterUser(const std::string& login, const std::string& password) {
    std::scoped_lock lock(mutex_);
    if (users_by_login_.count(login)) {
        throw std::runtime_error("Login already exists");
    }
    std::string user_name = "user" + std::to_string(users_by_login_.size() + 1);
    std::string hashed = HashPassword(password);

    int id = 0;
    std::string token = GenerateJWT("0", JWT_SECRET_KEY); // временный токен

    try {
        auto wrapper = pool_.GetConnection();
        pqxx::work w(*wrapper);
        auto res = w.exec_params(
            "INSERT INTO users (username, login, password_hash, token) VALUES ($1, $2, $3, $4) RETURNING id;",
            user_name, login, hashed, token);
        if (res.empty()) throw std::runtime_error("No ID returned");
        id = res[0][0].as<int>();
        w.commit();

        // Генерируем правильный JWT с реальным id
        token = GenerateJWT(std::to_string(id), JWT_SECRET_KEY);
        auto w2 = pool_.GetConnection();
        pqxx::work upd(*w2);
        upd.exec_params("UPDATE users SET token = $1 WHERE id = $2;", token, id);
        upd.commit();
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Registration failed: " << e.what();
        throw;
    }

    User user(login, hashed, id, user_name);
    user.SetToken(token);
    users_by_login_.emplace(login, std::move(user));
    users_by_id_.emplace(id, std::move(user));
    return token;
}

void User::SetToken(const std::string& token) {
    token_ = token;
}

User* Users::FindUserByToken(const std::string& token) {
    std::scoped_lock lock(mutex_);
    std::string user_id_str;
    if (!VerifyJWT(token, JWT_SECRET_KEY, user_id_str)) {
        return nullptr;
    }
    int user_id = std::stoi(user_id_str);
    return FindUserById(user_id);
}

User* Users::FindUserById(int id) {
    std::scoped_lock lock(mutex_);
    auto it = users_by_id_.find(id);
    return (it != users_by_id_.end()) ? &it->second : nullptr;
}

User* Users::FindUserByUserName(const std::string& user_name) {
    std::scoped_lock lock(mutex_);
    for (auto& [login, user] : users_by_login_) {
        if (user.GetUserName() == user_name) {
            return &user;
        }
    }
    return nullptr;
}

User* Users::FindUserByLogin(const std::string& login) {
    std::scoped_lock lock(mutex_);
    auto it = users_by_login_.find(login);
    return (it != users_by_login_.end()) ? &it->second : nullptr;
}
