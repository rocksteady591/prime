#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core.hpp>
#include <iostream>
#include <sodium/core.h>
#include <sodium/crypto_box.h>
#include <thread>
#include <sodium.h>
#include <cstring>
#include <random>
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

const std::pair<std::vector<unsigned char>, std::vector<unsigned char>> Client::generate_keypair(){
    unsigned char pk[crypto_box_PUBLICKEYBYTES];
    unsigned char sk[crypto_box_SECRETKEYBYTES];
    crypto_box_keypair(pk, sk);
    const std::vector<unsigned char>public_key(pk, pk + crypto_box_PUBLICKEYBYTES);
    const std::vector<unsigned char>private_key(sk, sk + crypto_box_SECRETKEYBYTES);
    return {public_key, private_key};
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
    std::string input;
    while(true){
        std::cout << "You: ";
        std::getline(std::cin, input);
        if(input == "exit"){break;}
        client.SendMessage(input);
        std::string response = client.Recieve();
        std::cout << "Server: " << response << std::endl;
    }
}
