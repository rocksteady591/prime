#include "request_handler.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>

RequestHandler::RequestHandler(const std::string& static_path)
    : static_path_(static_path) {}

RequestHandler::HttpResponse RequestHandler::SendBadRequest(const HttpRequest& request, const http::status status) {
    std::shared_ptr<http::response<http::string_body>> s_response = std::make_shared<http::response<http::string_body>>();
    s_response->result(http::status::not_found);
    s_response->insert(http::field::content_type, "text/plain");
    s_response->body() = (status == http::status::not_found) ? "404 File not found." : "400 Bad request.";
    s_response->prepare_payload();
    s_response->keep_alive(request.keep_alive());
    return s_response;
}

RequestHandler::HttpResponse RequestHandler::request_handler(const HttpRequest& request) {
    if (request.method() != http::verb::get && request.method() != http::verb::head && request.method() != http::verb::post) {
        //add log
        return SendBadRequest(request, http::status::bad_request);
    }
    if (request.method() == http::verb::get) {
        std::string target(request.target().begin(), request.target().end());
        std::string decoded_url = DecodedURL(target);
        if (decoded_url == "/") {
            decoded_url = "/index.html";
        }
        if (!decoded_url.empty() && decoded_url[0] == '/') {
            decoded_url = decoded_url.substr(1);
        }

        std::filesystem::path receive_path =
            std::filesystem::weakly_canonical(std::filesystem::path(static_path_) / decoded_url);

        if (!IsSubpath(receive_path)) {
            //add log
            return SendBadRequest(request, http::status::not_found);
        }

        //add content type
        std::string content_type = ContentType(receive_path);
        return SendResponse(http::status::ok, std::move(request), content_type, receive_path.string());
    }
    else {

    }
}

RequestHandler::HttpResponse RequestHandler::SendResponse(const http::status status, const HttpRequest& request, const std::string& content_type, const std::filesystem::path& full_path) {
    //add log
    std::shared_ptr<http::response<http::file_body>> response = std::make_shared<http::response<http::file_body>>();
    response->version(request.version());
    
    response->insert(http::field::content_type, content_type);
    http::file_body::value_type file;
    boost::system::error_code ec;
    file.open(full_path.string().c_str(), beast::file_mode::read, ec);

    if (ec) {
        //add log
        std::cerr << "File dont open: " << ec.message() << std::endl;
        
        return SendBadRequest(request, http::status::not_found);
    }
    response->result(status);
    response->body() = std::move(file);
    response->prepare_payload();
    response->keep_alive(request.keep_alive());
    return response;
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