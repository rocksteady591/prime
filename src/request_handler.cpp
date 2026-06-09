#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <boost/json.hpp>
#include <boost/json/object.hpp>
#include "boost/json/serialize.hpp"
#include <boost/log/utility/manipulators/add_value.hpp>
#include "request_handler.h"
#include "log.h"

namespace json = boost::json;
namespace logging = boost::log;

RequestHandler::RequestHandler(const std::string& static_path)
    : static_path_(static_path) {}

void RequestHandler::LogHandler(std::size_t error_code, std::string data, std::string message){
    json::object obj;
    obj["code"] = error_code;
    obj["data"] = std::move(data);
    BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj) << logging::add_value("msg", std::move(message));
}

RequestHandler::HttpResponse RequestHandler::request_handler(HttpRequest request) {
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

    std::string target(request.target().begin(), request.target().end());
    std::string decoded_url = DecodedURL(target);
    if (decoded_url == "/api/login") {
        if (request.method() != http::verb::post) {
            json::object obj;
			obj["code"] = "invalidMethod";
            obj["message"] = "Only POST method is expected";
            auto response = text_response(http::status::method_not_allowed, json::serialize(obj));
            response->set(http::field::allow, "POST");
            LogHandler(405, "invalidMethod"s, "Only POST method is expected"s);
            return response;
        }
        if (request[http::field::content_type] != "application/json") {
            json::object obj;
            obj["code"] = "invalidArgument";
            obj["message"] = "Request parse error";
            LogHandler(400, "invalidArgument"s, "Request parse error"s);
            return text_response(http::status::bad_request, json::serialize(obj));
        }
        std::string login;
        std::string password;
        try
        {
            json::value request_body = json::parse(request.body());
            json::object req_obj = request_body.as_object();
            login = req_obj["login"].as_string();
            password = req_obj["password"].as_string();
        }
        catch(const std::exception& e)
        {
            json::object obj;
            obj["code"] = "invalidArgument";
            obj["message"] = "Request parse error";
            LogHandler(400, "invalidArgument"s, "Request parse error"s);
            return text_response(http::status::bad_request, json::serialize(obj));
        }
        if(login.empty()){
            json::object obj;
            obj["code"] = "invalidArgument";
            obj["message"] = "Login is empty";
            LogHandler(400, "invalidArgument"s, "Login is empty"s);
            return text_response(http::status::bad_request, json::serialize(obj));
        }
        if(password.empty()){
            json::object obj;
            obj["code"] = "invalidArgument";
            obj["message"] = "Password is empty";
            LogHandler(400, "invalidArgument"s, "Password is empty"s);
            return text_response(http::status::bad_request, json::serialize(obj));
        }
        
    }
    if (!decoded_url.empty() && decoded_url[0] == '/') {
        decoded_url = decoded_url.substr(1);
    }

    std::filesystem::path receive_path =
        std::filesystem::weakly_canonical(std::filesystem::path(static_path_) / decoded_url);

    if (!IsSubpath(receive_path)) {
        //add log
        //return SendBadRequest(request, http::status::not_found);
    }

    //add content type
    std::string content_type = ContentType(receive_path);
    //return SendResponse(http::status::ok, std::move(request), content_type/*, receive_path.string()*/);

}

std::string RequestHandler::DecodedURL(const std::string& target) {
    std::string res;
    res.reserve(target.size());
    for (size_t i = 0; i < target.size(); ++i) {
        if (target[i] == '%' && i + 2 < target.size()) {
            std::string hex_str = target.substr(i + 1, 2);
            res.push_back(static_cast<char>(std::strtol(hex_str.c_str(), nullptr, 16)));
            i += 2;
        }
        else if (target[i] == '+') {
            res.push_back(' ');
        }
        else {
            res.push_back(target[i]);
        }
    }
    return res;
}

bool RequestHandler::IsSubpath(const std::filesystem::path& receive_path) {
    std::filesystem::path base_path = std::filesystem::weakly_canonical(static_path_);
    //receive_path = std::filesystem::weakly_canonical(receive_path);
    auto [first, last] = std::mismatch(base_path.begin(), base_path.end(),
        receive_path.begin(), receive_path.end());
    return first == base_path.end();
}

const std::string RequestHandler::ContentType(const std::filesystem::path& path) {
    std::string type = path.extension().string();
    std::transform(type.begin(), type.end(), type.begin(), [](unsigned char c) { return tolower(c); });

    if (type == ".htm" || type == ".html") return "text/html";
    if (type == ".css")                    return "text/css";
    if (type == ".txt")                    return "text/plain";
    if (type == ".js")                     return "text/javascript";
    if (type == ".json")                   return "application/json";
    if (type == ".xml")                    return "application/xml";
    if (type == ".png")                    return "image/png";
    if (type == ".jpg" || type == ".jpe" || type == ".jpeg") return "image/jpeg";
    if (type == ".gif")                    return "image/gif";
    if (type == ".bmp")                    return "image/bmp";
    if (type == ".ico")                    return "image/vnd.microsoft.icon";
    if (type == ".tiff" || type == ".tif") return "image/tiff";
    if (type == ".svg" || type == ".svgz") return "image/svg+xml";
    if (type == ".mp3")                    return "audio/mpeg";
    return "application/octet-stream";
}