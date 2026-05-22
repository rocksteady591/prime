#pragma once
#include <unordered_map>
#include <memory>
#include <thread>
#include <boost/beast.hpp>
#include <vector>

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
    void key_exchange(const std::vector<unsigned char>& received_key);
    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
    std::vector<unsigned char> shared_secret_key_;
    std::vector<unsigned char> sk_;
};

class Server{
public:
    explicit Server();
    void RunServer();

private:
    unsigned short threads_count_;
    const unsigned short port_ = 8080;
    net::io_context io_context_{threads_count_};
    tcp::acceptor acceptor_;
    //std::unordered_map<std::string, std::shared_ptr<User>> users_;
    std::vector<std::thread> thread_pool_;
    void do_accept();
    std::vector<unsigned char> sk_;
    std::vector<unsigned char> shared_secret_key_;
};