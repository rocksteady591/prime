#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
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

    HttpResponse request_handler(HttpRequest request);

private:
    std::string static_path_;
    const std::string ContentType(const std::filesystem::path& path);
    std::string DecodedURL(const std::string& target);
    bool IsSubpath(const std::filesystem::path& path);
    void LogHandler(std::size_t error_code, std::string data, std::string message);
};