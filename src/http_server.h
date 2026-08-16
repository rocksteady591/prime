#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip//address.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>
#include <pqxx/pqxx>

#include <memory>
#include <variant>

#include "chat.h"
#include "request_handler.h"
#include "user.h"

namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace http = boost::beast::http;
namespace beast = boost::beast;

class Session : public std::enable_shared_from_this<Session> {
    using HttpRequest = http::request<http::string_body>;
    using HttpResponse = std::variant <std::shared_ptr< http::response<http::file_body >>, std::shared_ptr<http::response<http::string_body>>>;
public:
    Session(tcp::socket&& socket, net::ssl::context& ctx ,Users& users, ChatManager& chat_manager);
    void Run();
private:
    RequestHandler handler_;
    void Read();
    void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_transfered);
    void Write(HttpResponse response);
    void OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_transfered);
    beast::ssl_stream<beast::tcp_stream> stream_;
    beast::flat_buffer buffer_;
    HttpRequest request_;
    Users& users_;
};

class Listener : public std::enable_shared_from_this<Listener> {
public:
    Listener(net::io_context& ioc,
        const tcp::endpoint& endpoint,
        net::ssl::context& ctx,
        Users& users,
        ChatManager& chat_manager);
    void RunServer();
private:
    void DoAccept();
    //add report error method
    void OnnAccept(boost::system::error_code ec, tcp::socket socket);
    void AsyncRunServer(tcp::socket socket);
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    net::ssl::context& ctx_;
    Users& users_;
    ChatManager& chat_manager_;
};
