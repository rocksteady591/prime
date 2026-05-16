#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <string>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = net::ip::tcp;

class Client{
public:
    Client(const std::string& host, const std::string& port);
    void Connect();
    void SendMessage(const std::string& message);
    std::string Recieve();
private:
    const std::pair<std::vector<unsigned char>, std::vector<unsigned char>> generate_keypair();
    std::string host_;
    std::string port_;
    net::io_context io_context_;
    tcp::resolver resolver_;
    websocket::stream<tcp::socket> ws_;
};