#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/registered_buffer.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/buffer.hpp>
#include <cstddef>
#include <iostream>
#include <sodium/core.h>
#include <sodium/crypto_box.h>
#include <thread>
#include <sodium.h>
#include <cstring>
#include <random>
#include <vector>
#include "client.h"
#include "user.h"
#include "message.pb.h"

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

std::vector<unsigned char> Client::compute_shared_key(
        const std::vector<unsigned char>& my_sk,
        const std::vector<unsigned char>& other_pk){
        unsigned char shared_secret_key[crypto_box_BEFORENMBYTES];
        crypto_box_beforenm(shared_secret_key, other_pk.data(), my_sk.data());
        return std::vector<unsigned char>(shared_secret_key, shared_secret_key + crypto_box_BEFORENMBYTES);
    }

const std::pair<std::vector<unsigned char>, std::vector<unsigned char>> Client::generate_keypair(){
    unsigned char pk[crypto_box_PUBLICKEYBYTES];
    unsigned char sk[crypto_box_SECRETKEYBYTES];
    crypto_box_keypair(pk, sk);
    const std::vector<unsigned char>public_key(pk, pk + crypto_box_PUBLICKEYBYTES);
    const std::vector<unsigned char>private_key(sk, sk + crypto_box_SECRETKEYBYTES);
    return {public_key, private_key};
}

void Client::key_exchange(){
    const auto [pk, sk] = generate_keypair();
    ws_.async_write(
        net::buffer(pk.data(), pk.size()), [](beast::error_code ec, std::size_t bytes_transfered){
            if(ec){
                std::cerr << "Ошибка отправки ключа: " << ec.message() << std::endl;
                return;
            }
            std::cout << "Ключ отправлен\n";
    });
    
    ws_.async_read(buffer_, [this, sk](beast::error_code ec, std::size_t bytes_received){
        if(ec){
            std::cerr << "Ключ не получен: " << ec.message() << std::endl;
            return;
        }
        std::cout << "Ключ получен\n";
        std::vector<unsigned char> received_key(bytes_received);
        net::buffer_copy(
        net::buffer(received_key.data(), received_key.size()), 
        buffer_.data()
        );
        shared_secret_key_ = compute_shared_key(sk, received_key);
        buffer_.consume(bytes_received);
    });
    
}

void Client::Run(){
    

    std::string input;
    while(true){
        std::cout << "You: ";
        std::getline(std::cin, input);
        if(input == "exit"){break;}
        SendMessage(input);
        std::string response = Recieve();
        std::cout << "Server: " << response << std::endl;
    }
}


void Client::SendMessage(const std::string& message){
    //messenger::SecureEnvelope envelope;
    //envelope.set_nonce(GenerateNonce());
    //envelope.set_ciphertext(message);
    //envelope.set_sender_id("pensil1");
    //std::string buffer;
    //envelope.SerializeToString(&buffer);
    //beast::error_code ec;
    //ws_.write(net::buffer(buffer), ec);
    //if(ec){
    //    std::cerr << "Error: " << ec.message() << std::endl;
    //}
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
    if(sodium_init() < 0){
        std::cout << "Libsodium not init\n";
        return 1;
    }
    Client client("127.0.0.1", "8080");
    client.Connect();
    std::cout << "Connected to server!" << std::endl;
    client.Run();
}
