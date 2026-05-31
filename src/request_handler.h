#pragma once

//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0A00
//#endif

#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <filesystem>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;

class RequestHandler {
public:
    using HttpRequest = http::request<http::string_body>;
    using HttpResponse = http::response<http::string_body>;

    RequestHandler(const std::string& static_path);

    std::shared_ptr<HttpResponse> request_handler(HttpRequest request);

private:
    std::string static_path_;

    std::shared_ptr<HttpResponse> SendResponse(const http::status status, const HttpRequest request, const std::string& content_type);
    std::string DecodedURL(const std::string& target);
    bool IsSubpath(const std::filesystem::path& path);
};