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
    void Run();
private:
    //void Write();
    //void OnWrite();
    //void Read();
    //void OnRead();
    void key_exchange();
    void SendMessage(const std::string& message);
    std::string Recieve();
    std::string host_;
    std::string port_;
    net::io_context io_context_;
    tcp::resolver resolver_;
    websocket::stream<tcp::socket> ws_;
    std::vector<unsigned char> sk_;
    std::vector<unsigned char> shared_secret_key_;
    beast::flat_buffer buffer_;
};