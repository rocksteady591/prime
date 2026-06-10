#include "http_server.h"
#include <algorithm>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip//address.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/beast/http.hpp>
#include <cstddef>
#include <exception>
#include <memory>
#include <thread>
#include <iostream>
#include <vector>
#include <variant>
#include "log.h"

namespace net = boost::asio;
namespace http = boost::beast::http;
using tcp = net::ip::tcp;


Session::Session(tcp::socket&& socket, const std::string& base)
    : stream_(std::move(socket)), handler_(RequestHandler(base)) {}

void Session::Run() {
    Read();
}

void Session::Read() {
    http::async_read(stream_, buffer_, request_,
        beast::bind_front_handler(&Session::OnRead, this->shared_from_this()));
}

void Session::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_transfered) {
    if (ec) {
        json::object obj;
        obj["Error"] = "readingError";
        BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj) << logging::add_value("msg", ec.message());
        return;
    }
    HttpResponse response = handler_.request_handler(std::move(request_));
    Write(response);
}

void Session::Write(HttpResponse response) {
    std::visit([this](auto&& arg) {
        http::async_write(stream_, *arg, [arg, self = shared_from_this()](beast::error_code ec, std::size_t bytes_transfered) {
            self->OnWrite(arg->need_eof(), ec, bytes_transfered);
            });
        }, response);
    
}

void Session::OnWrite(bool closed, beast::error_code ec, [[maybe_unused]] std::size_t bytes_transfered) {
    if (ec) {
        json::object obj;
        obj["Error"] = "writingError";
        BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj) << logging::add_value("msg", ec.message());
    }
    if (closed) {
        beast::error_code ecd;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
        if (ec) {
            json::object obj;
            obj["Error"] = "closeError";
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj) << logging::add_value("msg", ec.message());
        }
    }
    Read();
}

Listener::Listener(net::io_context& ioc, const tcp::endpoint& endpoint, const std::string& base)
    : ioc_(ioc), acceptor_(net::make_strand(ioc_)), base_path_(base) {
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(net::socket_base::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen(net::socket_base::max_listen_connections);
}

void Listener::RunServer() {
    DoAccept();
}

void Listener::DoAccept() {
    acceptor_.async_accept(ioc_.get_executor(), beast::bind_front_handler(&Listener::OnnAccept, this->shared_from_this()));
}
void Listener::OnnAccept(boost::system::error_code ec, tcp::socket socket) {
    if (ec) {
        json::object obj;
        obj["Error"] = "acceptError";
        BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj) << logging::add_value("msg", ec.message());
    }
    AsyncRunServer(std::move(socket));
    DoAccept();
}
void Listener::AsyncRunServer(tcp::socket socket) {
    std::make_shared<Session>(std::move(socket), base_path_)->Run();
}

template<typename T>
void RunWorkers(unsigned n, const T& fn) {
    std::vector<std::jthread> workers;
    int max = std::max(1u, n);
    workers.reserve(max - 1);
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

int main(/*int argc, const char* argv[]*/) {
    /*if (argc != 1) {
        std::cerr << "Dont include static" << std::endl;
        return 1;
    }*/
    using namespace std::literals;
    constexpr net::ip::port_type port = 80;
    try {
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);
        tcp::endpoint endpoint{ tcp::v4(), port };
        json::object obj;
        obj["port"] = port;
        obj["address"] = endpoint.address().to_string();
        BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj) << logging::add_value("msg", "server has started"s);
        const std::string base_path = "C:\\Users\\rocks\\source\\repos\\rocksteady591\\primal\\static"/*argv[0]*/;
        std::make_shared<Listener>(ioc, endpoint, base_path)->RunServer();

        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
            });
    }
    catch (const std::exception& ex) {
        json::object obj;
        obj["Error"] = "dntStartServer";
        BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj) << logging::add_value("msg", ex.what());
    }
}