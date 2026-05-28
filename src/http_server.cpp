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
#include <boost/beast/http.hpp>
#include <cstddef>
#include <exception>
#include <memory>
#include <thread>

namespace net = boost::asio;
namespace http = boost::beast::http;


Session::Session(tcp::socket&& socket) : stream_(std::move(socket)){}

void Session::Run(){
    DoRead();
}

void Session::DoRead(){
    http::async_read(stream_, buffer_, request_, 
    beast::bind_front_handler(&Session::OnRead, this->shared_from_this()));
}

void Session::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_transfered){
    if(ec){
        //add logging
        return;
    }
    
}

Listener::Listener(net::io_context& ioc, const tcp::endpoint& endpoint) 
    : ioc_(ioc), acceptor_(net::make_strand(ioc_)){
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(net::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen(net::socket_base::max_listen_connections);
    }

void Listener::RunServer(){
    DoAccept();
}

void Listener::DoAccept(){
    acceptor_.async_accept(ioc_.get_executor(), beast::bind_front_handler(&Listener::OnnAccept, this->shared_from_this()));
}
void Listener::OnnAccept(boost::system::error_code ec, tcp::socket socket){
    if(ec){
        return;
    }
    AsyncRunServer(std::move(socket));
    DoAccept();
}
void Listener::AsyncRunServer(tcp::socket socket){
    std::make_shared<Session>(std::move(socket))->Run();
}

int main(){
    constexpr net::ip::port_type port = 8080;
    try {
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);
        //InitLog
        tcp::endpoint endpoint{tcp::v4(), port};
        std::make_shared<Listener>(ioc, endpoint)->RunServer();
        
    } catch (const std::exception& ex) {
    
    }
}