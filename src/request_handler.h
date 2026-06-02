#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <filesystem>
#include <string>
#include <variant>

namespace beast = boost::beast;
namespace http = beast::http;

class RequestHandler {
public:
    using HttpRequest = http::request<http::string_body>;
    using HttpResponse = std::variant <std::shared_ptr< http::response<http::file_body >>, std::shared_ptr<http::response<http::string_body>>>;

    RequestHandler(const std::string& static_path);

    HttpResponse request_handler(const HttpRequest& request);

private:
    std::string static_path_;
    HttpResponse SendBadRequest(const HttpRequest& request, const http::status status);
    const std::string ContentType(const std::filesystem::path& path);
    HttpResponse SendResponse(const http::status status, const HttpRequest& request, const std::string& content_type, const std::filesystem::path& full_path);
    std::string DecodedURL(const std::string& target);
    bool IsSubpath(const std::filesystem::path& path);
};