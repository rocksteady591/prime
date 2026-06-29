#pragma once
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <boost/json/object.hpp>
#include "boost/json/serialize.hpp"
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <filesystem>
#include <string>
#include <variant>
#include "user.h"

namespace beast = boost::beast;
namespace json = boost::json;
namespace http = beast::http;

class RequestHandler {
public:
    using HttpRequest = http::request<http::string_body>;
    using HttpResponse = std::variant <std::shared_ptr< http::response<http::file_body >>, std::shared_ptr<http::response<http::string_body>>>;

    RequestHandler(Users& users);

    HttpResponse HandleApiPost(HttpRequest request);

private:
    Users& users_;

    
    template <typename F>
    HttpResponse HandleRegister(const HttpRequest& request, F&& response) {
        using namespace std::literals;
        std::string login;
        std::string pass_hash;
        try {
            auto body = boost::json::parse(request.body()).as_object();
            login = std::move(body["login"].as_string().c_str());
            pass_hash = std::move(body["password_hash"].as_string().c_str());
        }
        catch (const std::exception& e) {
            json::object obj;
            obj["code"] = "invalidArgument";
            obj["message"] = "Request parse error";
            LogHandler(400, "invalidArgument"s, "Request parse error: "s + e.what());
            return response(http::status::bad_request, json::serialize(obj));
        }
        if (login.empty() || pass_hash.empty()) {
            json::object obj;
            obj["code"] = "invalidArgument";
            obj["message"] = "login and password_hash required";
            LogHandler(400, "invalidArgument"s, "login and password_hash required"s);
            return response(http::status::bad_request, json::serialize(obj));
        }

        std::string token = users_.RegisterUser(login, pass_hash);
        boost::json::object resp;
        resp["token"] = token;
        resp["user_id"] = std::to_string(users_.FindUserByLogin(login)->GetId());
        LogHandler(200, "Register"s, "User is registered"s);
        return response(http::status::ok, json::serialize(resp));
    }

    template <typename F>
    HttpResponse HandleLogin(const HttpRequest& request, F&& response) {
        using namespace std::literals;
        std::string login;
        std::string pass_hash;
        try {
            auto body = boost::json::parse(request.body()).as_object();
            login = std::move(body["login"].as_string().c_str());
            pass_hash = std::move(body["password_hash"].as_string().c_str());
        }
        catch (const std::exception& e) {
            json::object obj;
            obj["code"] = "invalidArgument";
            obj["message"] = "Request parse error";
            LogHandler(400, "invalidArgument"s, "Request parse error"s);
            return response(http::status::bad_request, json::serialize(obj));
        }
        if (login.empty()) {
            json::object obj;
            obj["code"] = "invalidArgument";
            obj["message"] = "Login is empty";
            LogHandler(400, "invalidArgument"s, "Login is empty"s);
            return response(http::status::bad_request, json::serialize(obj));
        }
        auto* user = users_.FindUserByLogin(login);
        if (!user || user->GetPasswordHash() != pass_hash) {
            json::object obj;
            obj["code"] = "invalidArgument";
            obj["message"] = "Invalid credentials";
            LogHandler(400, "Invalid credentials"s, "User not found"s);
            return response(http::status::unauthorized, json::serialize(obj));
        }

        boost::json::object resp;
        resp["token"] = user->GetToken();
        resp["user_id"] = std::to_string(user->GetId());
        LogHandler(200, "Auth"s, "User is autorized"s);
        return response(http::status::ok, json::serialize(resp));
    }
    template <typename F>
    HttpResponse HandleFindUser(const HttpRequest& request, F&& response) {
        using namespace std::literals;
        std::string user_name;
        try
        {
            json::value body_val = json::parse(request.body());
            json::object body_obj = body_val.as_object();
            user_name = std::move(body_obj["login"].as_string());
        }
        catch (const std::exception& e)
        {
            json::object obj;
            obj["code"] = "invalidArgument";
            obj["message"] = "Request parse error";
            LogHandler(400, "invalidArgument"s, "Request parse error: "s + e.what());
            return response(http::status::bad_request, json::serialize(obj));
        }
        if (user_name.empty()) {
            json::object obj;
            obj["code"] = "invalidArgument";
            obj["message"] = "Login is empty";
            LogHandler(400, "invalidArgument"s, "Login is empty"s);
            return response(http::status::bad_request, json::serialize(obj));
        }
        auto* user = users_.FindUserByUserName(user_name);
        if (!user) {
            json::object obj;
            obj["code"] = "invalidArgument";
            obj["message"] = "Invalid credentials";
            LogHandler(400, "Invalid credentials"s, "User not found"s);
            return response(http::status::unauthorized, json::serialize(obj));
        }
        json::object body_response;
        body_response["user_login"] = user->GetLogin();
        LogHandler(200, "containsUser"s, "User is contains"s);
        return response(http::status::ok, json::serialize(body_response));
    }
    void LogHandler(std::size_t error_code, std::string data, std::string message);
};