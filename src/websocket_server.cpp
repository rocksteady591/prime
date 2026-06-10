#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <cstddef>
#include <sodium/core.h>
#include <sodium/crypto_box.h>
#include <thread>
#include <iostream>
#include <iterator>
#include <sodium.h>
#include <vector>
#include "websocket_server.h"
#include "message.pb.h"
#include "log.h"

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
namespace logging = boost::log;
namespace keywords = boost::log::keywords;
using tcp = net::ip::tcp;
using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;
using namespace std::literals;


void InitBoostLogFilter() {
    logging::add_console_log(
        std::clog,
        keywords::format = &MyFormatter,
        keywords::auto_flush = true
        );
}

void Server::LogActions(const std::string& data, const std::string& message) {
    
}

std::vector<unsigned char> compute_shared_key(
    const std::vector<unsigned char>& my_sk,
    const std::vector<unsigned char>& other_pk) {
    unsigned char shared_secret_key[crypto_box_BEFORENMBYTES];
    crypto_box_beforenm(shared_secret_key, other_pk.data(), my_sk.data());
    return std::vector(shared_secret_key, shared_secret_key + crypto_box_BEFORENMBYTES);
}

const std::pair<std::vector<unsigned char>, std::vector<unsigned char>> generate_keypair() {
    unsigned char pk[crypto_box_PUBLICKEYBYTES];
    unsigned char sk[crypto_box_SECRETKEYBYTES];
    crypto_box_keypair(pk, sk);
    const std::vector<unsigned char> public_key(pk, pk + crypto_box_PUBLICKEYBYTES);
    const std::vector<unsigned char> private_key(sk, sk + crypto_box_SECRETKEYBYTES);
    return { public_key, private_key };
}

Session::Session(tcp::socket socket) : ws_(std::move(socket)) {}

void Session::key_exchange(const std::vector<unsigned char>& received_key) {
    auto [pk, sk] = generate_keypair();
    ws_.async_write(net::buffer(pk.data(), pk.size()), [](beast::error_code ec, std::size_t bytes_write) {
        if (ec) {
            json::object obj;
            obj["Error"] = "pb_keyDontSend";
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj) << logging::add_value("msg", ec.message());
        }
        });
    sk_ = std::move(sk); 
    shared_secret_key_ = compute_shared_key(sk_, received_key);
}

void Session::DoRead() {
    ws_.async_read(buffer_, [self = shared_from_this()](beast::error_code ec, std::size_t bytes_read) {
        if (ec) {
            json::object obj;
            obj["Error"] = "error read";
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj) << logging::add_value("msg", ec.message());
            return;
        }

        // Первый шаг: обмен ключами
        if (self->shared_secret_key_.empty()) {
            std::vector<unsigned char> received_key(bytes_read);
            net::buffer_copy(net::buffer(received_key.data(), received_key.size()),
                self->buffer_.data());
            self->key_exchange(received_key);
            self->buffer_.consume(bytes_read);
            self->DoRead();
            return;
        }

        // Получаем данные из буфера
        std::string data = beast::buffers_to_string(self->buffer_.data());
        self->buffer_.consume(self->buffer_.size());

        // Парсим SecureEnvelope
        messenger::SecureEnvelope recieve_envelope;
        if (!recieve_envelope.ParseFromString(data)) {
            json::object obj;
            obj["Warning"] = "parseFailed";
            BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj) << logging::add_value("msg", "Failed to parse SecureEnvelope"s);
            return;
        }

        const std::string& ciphertext = recieve_envelope.ciphertext();
        const std::string& nonce = recieve_envelope.nonce();
        std::string sender_id = recieve_envelope.sender_id();

        /*std::cout << "Received envelope from " << sender_id << "\n";
        std::cout << "Ciphertext size: " << ciphertext.size() << "\n";
        std::cout << "Nonce size: " << nonce.size() << "\n";*/

        // Проверка минимальной длины шифротекста
        if (ciphertext.size() < crypto_box_MACBYTES) {
            json::object obj;
            obj["Warning"] = "shrotText";
            BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj) << logging::add_value("msg", "Ciphertext too short"s);
            return;
        }
        if (nonce.size() != crypto_box_NONCEBYTES) {
            json::object obj;
            obj["Warning"] = "invalidNonce";
            BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj) << logging::add_value("msg", "Invalid nonce size"s);
            return;
        }

        // Расшифровка
        std::vector<unsigned char> plaintext(ciphertext.size() - crypto_box_MACBYTES);
        if (crypto_box_open_easy_afternm(
            plaintext.data(),
            reinterpret_cast<const unsigned char*>(ciphertext.data()),
            ciphertext.size(),
            reinterpret_cast<const unsigned char*>(nonce.data()),
            self->shared_secret_key_.data()) != 0) {

            json::object obj;
            obj["Warning"] = "decryptFailed";
            BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj) << logging::add_value("msg", "Decryption failed"s);
            return;
        }

        std::string message(plaintext.begin(), plaintext.end());
        //std::cout << "Decrypted message: " << message << std::endl;

        // Формируем зашифрованный ответ (эхо)
        std::vector<unsigned char> new_nonce(crypto_box_NONCEBYTES);
        randombytes_buf(new_nonce.data(), new_nonce.size());

        std::vector<unsigned char> encrypted(message.size() + crypto_box_MACBYTES);
        crypto_box_easy_afternm(
            encrypted.data(),
            reinterpret_cast<const unsigned char*>(message.data()),
            message.size(),
            new_nonce.data(),
            self->shared_secret_key_.data());

        messenger::SecureEnvelope response;
        response.set_ciphertext(encrypted.data(), encrypted.size());
        response.set_nonce(new_nonce.data(), new_nonce.size());
        response.set_sender_id("Server");

        std::string serialized;
        response.SerializeToString(&serialized);

        auto sp = std::make_shared<std::string>(std::move(serialized));

        self->ws_.async_write(
            net::buffer(*sp),
            [self, sp](beast::error_code ec, std::size_t) {
                if (!ec) {
                    self->DoRead();
                }
                else {
                    json::object obj;
                    obj["Error"] = "writeError";
                    BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj) << logging::add_value("msg", ec.message());
                }
            });
        });
}
void Session::Run() {
    ws_.async_accept([self = shared_from_this()](beast::error_code ec) {
        if (ec) {
            json::object obj;
            obj["Error"] = "runError";
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj) << logging::add_value("msg", ec.message());
            return;
        }
        // ВСЕ ИСХОДЯЩИЕ СООБЩЕНИЯ ТЕПЕРЬ БИНАРНЫЕ
        self->ws_.binary(true);
        self->DoRead();
        });
}

Server::Server()
    : threads_count_(std::thread::hardware_concurrency()),
    io_context_(threads_count_),
    acceptor_(io_context_, tcp::endpoint{ tcp::v4(), port_ }) {}

void Server::do_accept() {
    acceptor_.async_accept([this](beast::error_code ec, tcp::socket socket) {
        if (!ec) {
            json::object obj;
            obj["address"] = socket.remote_endpoint().address().to_string();
            BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj) << logging::add_value("msg", "new connetction"s);

            std::make_shared<Session>(std::move(socket))->Run();
        }
        do_accept();
        });
}

void Server::RunServer() {
    json::object obj;
    obj["port"] = port_;
    obj["address"] = acceptor_.local_endpoint().address().to_string();
    BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj) << logging::add_value("msg", "server is run"s);
    do_accept();

    for (size_t i = 0; i < threads_count_; ++i) {
        thread_pool_.emplace_back([this] {
            io_context_.run();
            });
    }
    for (auto& t : thread_pool_) {
        if (t.joinable()) { t.join(); }
    }

}

int main() {
    if (sodium_init() < 0) {
        std::cerr << "Libsodium not init\n";
        return 1;
    }
    InitBoostLogFilter();
    Server server;
    server.RunServer();
}