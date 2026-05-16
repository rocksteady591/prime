#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <sodium/core.h>
#include <sodium/crypto_box.h>
#include <thread>
#include <iostream>
#include <sodium.h>
#include "server.h"
#include "message.pb.h"

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
using tcp = net::ip::tcp;
using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;
using namespace std::literals;


Session::Session(tcp::socket socket) : ws_(std::move(socket)) {}

void Session::DoRead(){
    ws_.async_read(buffer_, [self = shared_from_this()](beast::error_code ec, std::size_t){
        if(ec){
            return;
        }
        std::string data = beast::buffers_to_string(self->buffer_.data());
        self->buffer_.consume(self->buffer_.size());
        messenger::SecureEnvelope recieve_envelope;
        if(recieve_envelope.ParseFromString(data)){
            std::string encrypted_text = recieve_envelope.ciphertext();
            std::string nonce = recieve_envelope.nonce();
            std::string sender_id = recieve_envelope.sender_id();
        }else{

        }
        self->ws_.async_write(self->buffer_.data(), [self](beast::error_code ec, std::size_t){
            self->buffer_.consume(self->buffer_.size());
            if(!ec){
                self->DoRead();
            }
        });
    });
}


void Session::Run(){
    ws_.async_accept([self = shared_from_this()](beast::error_code ec){
        if(ec){
            return;
        }
        self->DoRead();
    });
}

const std::pair<std::vector<unsigned char>, std::vector<unsigned char>> Server::generate_keypair(){
    unsigned char pk[crypto_box_PUBLICKEYBYTES];
    unsigned char sk[crypto_box_SECRETKEYBYTES];
    crypto_box_keypair(pk, sk);
    const std::vector<unsigned char> public_key(pk, pk + crypto_box_PUBLICKEYBYTES);
    const std::vector<unsigned char> private_key(sk, sk + crypto_box_SECRETKEYBYTES);
    return {public_key, private_key};
}

Server::Server(/*size_t threads_count = */) 
: threads_count_(std::thread::hardware_concurrency()), 
io_context_(threads_count_), 
acceptor_(io_context_, tcp::endpoint{tcp::v4(), port_}){}

void Server::do_accept(){
    acceptor_.async_accept([this](beast::error_code ec, tcp::socket socket){
        if(!ec){
            std::cout << "New connection: " << socket.remote_endpoint() << std::endl;
            std::make_shared<Session>(std::move(socket))->Run();
        }
        do_accept();
    });
}

void Server::RunServer(){
    std::cout << "Server has started on port: "sv << port_ << "..." << std::endl;
    do_accept();

    for(size_t i = 0; i < threads_count_; ++i){
        thread_pool_.emplace_back([this]{
            io_context_.run();
        });
    }
    for (auto& t : thread_pool_) {
        if (t.joinable()) {t.join();}
    }
    
}

int main(){
    if(sodium_init() < 0){
        std::cout << "Libsodium not init\n";
        return 1;
    }
    Server server; 
    server.RunServer();
}