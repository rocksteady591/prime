#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip//address.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/system/detail/error_code.hpp>
#include <memory>

#include "request_handler.h"

namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace http = boost::beast::http;
namespace beast = boost::beast;

class Session : public std::enable_shared_from_this<Session> {
    using HttpRequest = http::request<http::string_body>;
    using HttpResponse = http::response<http::string_body>;
public:
    Session(tcp::socket&& socket, const std::string& base);
    void Run();
private:
    RequestHandler handler_;
    void DoRead();
    void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_transfered);
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    HttpRequest request_;
};

class Listener : public std::enable_shared_from_this<Listener> {
public:
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, const std::string& base);
    void RunServer();
private:
    void DoAccept();
    void OnnAccept(boost::system::error_code ec, tcp::socket socket);
    void AsyncRunServer(tcp::socket socket);
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    std::string base_path_;
};
