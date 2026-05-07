#pragma once
#include "user.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = net::ip::tcp;

class Session : public std::enable_shared_from_this<Session>{
public:
    Session(tcp::socket socket);
    void Run();
    void DoRead();
private:
    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
};

class Server{
public:
    explicit Server(/*size_t threads_count = std::thread::hardware_concurrency()*/);
    void RunServer();

private:
    unsigned short threads_count_;
    const unsigned short port_ = 8080;
    net::io_context io_context_{threads_count_};
    tcp::acceptor acceptor_;
    std::unordered_map<std::string, std::shared_ptr<User>> users_;
    std::vector<std::thread> thread_pool_;
    void do_accept();
};