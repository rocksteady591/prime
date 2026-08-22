#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <boost/json.hpp>
#include <boost/json/object.hpp>
#include "boost/json/serialize.hpp"
#include <boost/log/utility/manipulators/add_value.hpp>
#include "request_handler.h"
#include "chat.h"
#include "log.h"

namespace json = boost::json;
namespace logging = boost::log;

RequestHandler::RequestHandler(Users& users, ChatManager& chat_manager): users_(users), chat_manager_(chat_manager) {}

RequestHandler::HttpResponse RequestHandler::HandleApiPost(HttpRequest request) {
    using namespace std::literals;
    auto text_response = [&](http::status status, std::string body, std::string_view type = "application/json") {
        std::shared_ptr<http::response<http::string_body>> response = std::make_shared<http::response<http::string_body>>(status, request.version());
        response->set(http::field::content_type, type);
        response->body() = std::move(body);
        response->set(http::field::cache_control, "no-cache");
        response->prepare_payload();
        response->keep_alive(request.keep_alive());
        return response;
        };
    if (request.method() != http::verb::post) {
        json::object obj;
        obj["code"] = "invalidMethod";
        obj["message"] = "Only POST method is expected";
        auto response = text_response(http::status::method_not_allowed, json::serialize(obj));
        response->set(http::field::allow, "POST");
        LogHandler(405, "invalidMethod"s, "Only POST method is expected"s);
        return response;
    }
    std::string target = request.target();
    if (target == "/api/register") {
        return HandleRegister(request, text_response);
    }
    else if (target == "/api/login") {
        return HandleLogin(request, text_response);
    }
    else if (target == "/api/find_user") {
        return HandleFindUser(request, text_response);
    }else if(target == "/api/get_messages"){
        return  HandleGetMessages(request, text_response);
    }else if(target == "/api/get_chats"){
        return HandleGetChats(request, text_response);
    }else if(target == "/api/get_contacts"){
        return HandleGetContacts(request, text_response);
    }else if(target == "api/logout"){
        return HandleLogout(request, text_response);
    }
    boost::json::object resp;
    resp["code"] = "invalidApiMethod";
    resp["message"] = "Unknown API endpoint";
    LogHandler(404, "invalidApiMethod"s, "Unknown API endpoint"s);
    return text_response(http::status::ok, json::serialize(resp));
}

void RequestHandler::LogHandler(std::size_t error_code, std::string data, std::string message){
    json::object obj;
    obj["code"] = error_code;
    obj["data"] = std::move(data);
    BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj) << logging::add_value("msg", std::move(message));
}
