#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core.hpp>
#include <iostream>
#include <thread>
#include "client.h"
#include "user.h"

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
using tcp = net::ip::tcp;
using namespace std::literals;

Client::Client(const std::string& host, const std::string& port) 
: host_(std::move(host)),
port_(std::move(port)),
resolver_(io_context_),
ws_(io_context_) {}

void Client::Connect(){
    const auto result = resolver_.resolve(host_, port_);
    net::connect(ws_.next_layer(), result.begin(), result.end());
    ws_.handshake(host_, "/");
}

void Client::SendMessage(const std::string& message){
    beast::error_code ec;
    ws_.write(net::buffer(message), ec);
    if(ec){
        std::cerr << "Error: " << ec.message() << std::endl;
    }
}

std::string Client::Recieve(){
    beast::flat_buffer buffer;
    beast::error_code ec;
    ws_.read(buffer, ec);

    if(ec){
        std::cerr << "Error: " << ec.message() << std::endl;
    }
    return beast::buffers_to_string(buffer.data());
}

int main(){
    Client client("127.0.0.1", "8080");
    client.Connect();
    std::cout << "Connected to server!" << std::endl;
    std::string input;
    while(true){
        std::cout << "You: ";
        std::getline(std::cin, input);
        if(input == "exit"){break;}
        client.SendMessage(input);
        std::string response = client.Recieve();
    }   
}