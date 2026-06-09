#pragma once
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>
#include <boost/json/object.hpp>
#include <boost/log/core/record_view.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/formatting_ostream_fwd.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/json.hpp>
#include <boost/log/utility/value_ref.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <thread>
#include <boost/beast.hpp>
#include <vector>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace logging = boost::log;
using tcp = net::ip::tcp;

inline void ServerLog(const std::string& data, const std::string& message) {

}

//inline void LogFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
//    using namespace std::literals;
//    boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
//    boost::json::object obj;
//    obj["timestamp"] = boost::posix_time::to_iso_extended_string(now);
//    logging::value_ref<boost::json::object>data = logging::extract<boost::json::object>("data", rec);
//    if (data) {
//        obj["data"] = data.get();
//    }
//    else {
//        obj["data"] = "empty data"s;
//    }
//    logging::value_ref<std::string>message = logging::extract<std::string>("msg", rec);
//    if (message) {
//        obj["message"] = message.get();
//    }
//    else {
//        obj["message"] = "empty message"s;
//    }
//
//}

class Session : public std::enable_shared_from_this<Session> {
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

class Server {
public:
    explicit Server();
    void RunServer();

private:
    unsigned short threads_count_;
    const unsigned short port_ = 9000;
    net::io_context io_context_{ threads_count_ };
    tcp::acceptor acceptor_;
    //std::unordered_map<std::string, std::shared_ptr<User>> users_;
    std::vector<std::thread> thread_pool_;
    void do_accept();
    std::vector<unsigned char> sk_;//?
    std::vector<unsigned char> shared_secret_key_;
    void LogActions(const std::string& data, const std::string& message);
};