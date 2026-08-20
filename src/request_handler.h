#pragma once
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include "boost/json/serialize.hpp"
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/json/parse.hpp>
#include <exception>
#include <memory>
#include <filesystem>
#include <string>
#include <variant>
#include "chat.h"
#include "connection_pool.h"
#include "user.h"

#ifdef GetUserName
#undef GetUserName
#endif

namespace beast = boost::beast;
namespace json = boost::json;
namespace http = beast::http;

class RequestHandler {
public:
    using HttpRequest = http::request<http::string_body>;
    using HttpResponse = std::variant <std::shared_ptr< http::response<http::file_body >>, std::shared_ptr<http::response<http::string_body>>>;

    RequestHandler(Users& users, ChatManager& chat_manager);

    HttpResponse HandleApiPost(HttpRequest request);

private:
    Users& users_;
    ChatManager& chat_manager_;

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
        std::string token;
        try {
            token = users_.RegisterUser(login, pass_hash);
        } catch (const std::exception& e) {
            json::object obj;
            obj["code"] = "registrationFailed";
            obj["message"] = e.what();
            LogHandler(400, "registrationFailed"s, e.what());
            return response(http::status::bad_request, json::serialize(obj));
        }

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
        catch (const std::exception&) {
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

    template<typename F>
    HttpResponse HandleGetChats(const HttpRequest& request, F&& response){
        using namespace std::literals;
        int user_id = 0;
        try{
            json::value body_val = json::parse(request.body());
            json::object body_obj = body_val.as_object();
            user_id = body_obj["user_id"].as_int64();
        }catch(const std::exception&){
            json::object obj;
            obj["code"] = "invalidArgument";
            obj["message"] = "Request parse error";
            LogHandler(400, "invalidArgument"s, "Request parse error"s);
            return response(http::status::bad_request, json::serialize(obj));
        }
        try
        {
            std::vector<ChatInfo> chats = chat_manager_.GetChats(user_id);
            json::array res_arr;
            for(const ChatInfo& chat : chats){
                json::object o_chat;
                o_chat["id"] = chat.id;
                o_chat["user1_id"] = chat.user1_id;
                o_chat["user2_id"] = chat.user2_id;
                o_chat["create_timestamp"] = chat.create_chat_time;
                res_arr.push_back(std::move(o_chat));
            }
            LogHandler(200, "GetChats"s, "Chats have been received"s);
            return response(http::status::ok, json::serialize(res_arr));
        }
        catch(const pqxx::sql_error& e)
        {
            json::object obj;
            obj["code"] = e.what();
            obj["message"] = e.query();
            LogHandler(400, "BadRequest"s, "Invalid request parametrs or data"s);
            return response(http::status::bad_request, json::serialize(obj));
        }catch(const pqxx::broken_connection& e){
            json::object obj;
            obj["code"] = e.what();
            obj["message"] = e.query();
            LogHandler(500, "InternalServerError"s, "Database is temporarily unavailable"s);
            return response(http::status::internal_server_error, json::serialize(obj));
        }catch (const std::exception& e){
            json::object obj;
            obj["code"] = e.what();
            obj["message"] = "An unexpected error occurred";
            LogHandler(500, "InternalServerError"s, "An unexpected error occurred"s);
            return response(http::status::internal_server_error, json::serialize(obj));
        }

    }


    template<typename F>
    HttpResponse HandleGetMessages(const HttpRequest& request, F&& response){
        using namespace std::literals;
        int count_messages = 0;
        int offset = 0;
        int chat_id = 0;
        try {
            json::value body_val = json::parse(request.body());
            json::object body_obj = body_val.as_object();
            count_messages = body_obj["count_messages"].as_int64();
            offset = body_obj["offset"].as_int64();
            chat_id = body_obj["chat_id"].as_int64();
        } catch (const std::exception&) {
            json::object obj;
            obj["code"] = "invalidArgument";
            obj["message"] = "Request parse error";
            LogHandler(400, "invalidArgument"s, "Request parse error"s);
            return response(http::status::bad_request, json::serialize(obj));
        }
        std::vector<Message> messages = chat_manager_.GetMessages(chat_id, count_messages, offset);
        json::array messages_arr;
        try {
            messages_arr.reserve(messages.size());
            for(const Message& msg : messages){
                json::object j_msg;
                j_msg["msg_id"] = msg.id;
                j_msg["chat_id"] = msg.chat_id;
                j_msg["sender_id"] = msg.sender_id;
                j_msg["text"] = msg.text;
                j_msg["send_time"] = msg.send_time;
                messages_arr.push_back(std::move(j_msg));
            }
        } catch (const pqxx::sql_error& e) {
            json::object obj;
            obj["code"] = e.what();
            obj["message"] = e.query();
            LogHandler(400, "BadRequest"s, "Invalid request parametrs or data"s);
            return response(http::status::bad_request, json::serialize(obj));
        }catch (const pqxx::broken_connection& e){
            json::object obj;
            obj["code"] = e.what();
            obj["message"] = e.query();
            LogHandler(500, "InternalServerError"s, "Database is temporarily unavailable"s);
            return response(http::status::internal_server_error, json::serialize(obj));
        }catch (const std::exception& e){
            json::object obj;
            obj["code"] = e.what();
            obj["message"] = "An unexpected error occurred";
            LogHandler(500, "InternalServerError"s, "An unexpected error occurred"s);
            return response(http::status::internal_server_error, json::serialize(obj));
        }
        LogHandler(200, "GetMessages"s, "Messages is contains"s);
        return response(http::status::ok, json::serialize(messages_arr));
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
        body_response["username"] = user->GetUserName();
        body_response["user_id"] = user->GetId();
        LogHandler(200, "containsUser"s, "User is contains"s);
        return response(http::status::ok, json::serialize(body_response));
    }
    void LogHandler(std::size_t error_code, std::string data, std::string message);
};
