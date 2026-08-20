#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/json.hpp>
#include <cstddef>
#include <sodium.h>
#include <memory>
#include <thread>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include "chat.h"
#include "connection_pool.h"
#include "user.h"

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace logging = boost::log;
namespace json = boost::json;
using tcp = net::ip::tcp;

class Server; // forward declaration

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, Server* server);
    ~Session();
    void Run();
    void SendRaw(const std::string& raw_data);
    void DoRead();
    void ForceClose();
private:
    void key_exchange(const std::vector<unsigned char>& received_key);
    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
    std::vector<unsigned char> shared_secret_key_;
    std::vector<unsigned char> sk_;
    std::string user_id_;
    Server* server_;
};

class Server {
public:
    explicit Server(Users& users, ChatManager& chat_manager);
    void RunServer();
    void RegisterSession(const std::string& user_id, std::shared_ptr<Session> session);
    void UnregisterSession(const std::string& user_id);
    std::shared_ptr<Session> FindSession(const std::string& user_id);
    Users& GetUsers();
    ChatManager& GetManager();
private:
    unsigned short threads_count_;
    const unsigned short port_ = 9000;
    net::io_context io_context_{ static_cast<int>(threads_count_) };
    tcp::acceptor acceptor_{ io_context_ };
    std::vector<std::thread> thread_pool_;
    std::unordered_map<std::string, std::shared_ptr<Session>> sessions_;
    std::mutex sessions_mutex_;
    void do_accept();
    Users& users_;
    ChatManager& chat_manager_;
};
