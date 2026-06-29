#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/json.hpp>
#include <cstddef>
#include <sodium.h>
#include <thread>
#include <iostream>
#include <iterator>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

#include "websocket_server.h"
#include "message.pb.h"
#include "log.h"
#include "user.h"

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
namespace logging = boost::log;
namespace keywords = logging::keywords;
namespace json = boost::json;
using tcp = net::ip::tcp;
using namespace std::literals;

void InitLog() {
    logging::add_console_log(
        std::clog,
        keywords::format = &MyFormatter
    );
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

Session::Session(tcp::socket socket, Server* server)
    : ws_(std::move(socket)), server_(server) {
}

Session::~Session() {
    if (!user_id_.empty()) {
        server_->UnregisterSession(user_id_);
    }
}

void Session::Run() {
    // Читаем HTTP-запрос апгрейда WebSocket
    beast::http::request<beast::http::string_body> upgrade_req;
    beast::error_code ec;
    beast::http::read(ws_.next_layer(), buffer_, upgrade_req, ec);
    if (ec) {
        json::object obj;
        obj["Error"] = "readUpgradeError";
        BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
            << logging::add_value("msg", ec.message());
        return;
    }

    // Извлекаем токен из URL (?token=...)
    std::string token;
    std::string target = upgrade_req.target();
    auto pos = target.find("?token=");
    if (pos != std::string::npos) {
        token = target.substr(pos + 7);
        // Отсекаем возможные дополнительные параметры
        auto end = token.find('&');
        if (end != std::string::npos) token = token.substr(0, end);
    }

    // Проверяем токен
    User* user = server_->GetUsers().FindUserByToken(token);
    if (!user) {
        json::object obj;
        obj["Warning"] = "invalidToken";
        BOOST_LOG_TRIVIAL(warning) << logging::add_value("data", obj)
            << logging::add_value("msg", "Invalid token");
        // Отправляем 401 и закрываем соединение
        beast::http::response<beast::http::string_body> res{
            beast::http::status::unauthorized, upgrade_req.version() };
        res.set(beast::http::field::server, "Messenger");
        res.set(beast::http::field::content_type, "text/plain");
        res.body() = "Invalid token";
        beast::http::write(ws_.next_layer(), res, ec);
        return;
    }

    // Сохраняем ID пользователя и регистрируем сессию
    user_id_ = std::to_string(user->GetId());
    server_->RegisterSession(user_id_, shared_from_this());

    // Теперь выполняем WebSocket handshake
    ws_.async_accept(upgrade_req, [self = shared_from_this()](beast::error_code ec) {
        if (ec) {
            json::object obj;
            obj["Error"] = "acceptError";
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                << logging::add_value("msg", ec.message());
            return;
        }
        self->ws_.binary(true);
        self->DoRead();
        });
}

void Session::key_exchange(const std::vector<unsigned char>& received_key) {
    auto [pk, sk] = generate_keypair();
    ws_.async_write(net::buffer(pk.data(), pk.size()),
        [](beast::error_code ec, std::size_t bytes_write) {
            if (ec) {
                json::object obj;
                obj["Error"] = "pb_keyDontSend";
                BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                    << logging::add_value("msg", ec.message());
            }
        });
    sk_ = std::move(sk);
    shared_secret_key_ = compute_shared_key(sk_, received_key);
}

void Session::SendRaw(const std::string& raw_data) {
    auto sp = std::make_shared<std::string>(raw_data);
    ws_.async_write(
        net::buffer(*sp),
        [self = shared_from_this(), sp](beast::error_code ec, std::size_t) {
            if (ec) {
                json::object obj;
                obj["Error"] = "writeError";
                BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                    << logging::add_value("msg", ec.message());
            }
        });
}

void Session::DoRead() {
    ws_.async_read(buffer_, [self = shared_from_this()](beast::error_code ec, std::size_t bytes_read) {
        if (ec) {
            json::object obj;
            obj["Error"] = "error read";
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                << logging::add_value("msg", ec.message());
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
            BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj)
                << logging::add_value("msg", "Failed to parse SecureEnvelope"s);
            self->DoRead();   // продолжаем чтение, не закрывая соединение
            return;
        }

        const std::string& ciphertext = recieve_envelope.ciphertext();
        const std::string& nonce = recieve_envelope.nonce();
        std::string sender_id = recieve_envelope.sender_id();
        std::string recipient_id = recieve_envelope.recipient_id();

        // Проверка минимальной длины шифротекста
        if (ciphertext.size() < crypto_box_MACBYTES) {
            json::object obj;
            obj["Warning"] = "shortText";
            BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj)
                << logging::add_value("msg", "Ciphertext too short"s);
            self->DoRead();
            return;
        }
        if (nonce.size() != crypto_box_NONCEBYTES) {
            json::object obj;
            obj["Warning"] = "invalidNonce";
            BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj)
                << logging::add_value("msg", "Invalid nonce size"s);
            self->DoRead();
            return;
        }

        // Расшифровка (только для валидации и получения sender_id)
        std::vector<unsigned char> plaintext(ciphertext.size() - crypto_box_MACBYTES);
        if (crypto_box_open_easy_afternm(
            plaintext.data(),
            reinterpret_cast<const unsigned char*>(ciphertext.data()),
            ciphertext.size(),
            reinterpret_cast<const unsigned char*>(nonce.data()),
            self->shared_secret_key_.data()) != 0) {

            json::object obj;
            obj["Warning"] = "decryptFailed";
            BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj)
                << logging::add_value("msg", "Decryption failed"s);
            self->DoRead();
            return;
        }
        std::string message(plaintext.begin(), plaintext.end());  // <--

        // Регистрируем сессию, если ещё не зарегистрирована
        //if (self->user_id_.empty() && !sender_id.empty()) {
        //    self->user_id_ = sender_id;
        //    self->server_->RegisterSession(sender_id, self->shared_from_this());
        //    json::object obj;
        //    obj["userId"] = sender_id;
        //    BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj)
        //        << logging::add_value("msg", "Session registered"s);
        //}

        // Пересылаем зашифрованное сообщение получателю
        if (!recipient_id.empty()) {
            auto target = self->server_->FindSession(recipient_id);
            if (target) {
                // Перешифровываем для получателя
                std::vector<unsigned char> new_nonce(crypto_box_NONCEBYTES);
                randombytes_buf(new_nonce.data(), new_nonce.size());

                std::vector<unsigned char> encrypted(message.size() + crypto_box_MACBYTES);
                crypto_box_easy_afternm(
                    encrypted.data(),
                    reinterpret_cast<const unsigned char*>(message.data()),
                    message.size(),
                    new_nonce.data(),
                    target->shared_secret_key_.data());   // ключ получателя

                messenger::SecureEnvelope forward_env;
                forward_env.set_ciphertext(encrypted.data(), encrypted.size());
                forward_env.set_nonce(new_nonce.data(), new_nonce.size());
                forward_env.set_sender_id(sender_id);
                forward_env.set_recipient_id(recipient_id);

                std::string serialized;
                forward_env.SerializeToString(&serialized);
                target->SendRaw(serialized);

                json::object obj;
                obj["from"] = sender_id;
                obj["to"] = recipient_id;
                BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj)
                    << logging::add_value("msg", "Message forwarded (re-encrypted)");
            }
            else {
                // Получатель не в сети
                json::object obj;
                obj["recipient"] = recipient_id;
                BOOST_LOG_TRIVIAL(warning) << logging::add_value("data", obj)
                    << logging::add_value("msg", "Recipient offline");
            }
        }

        // Продолжаем чтение
        self->DoRead();
        });
}

Server::Server(Users& users)
    :   threads_count_(std::thread::hardware_concurrency()),
        io_context_(threads_count_),
        acceptor_(io_context_, tcp::endpoint{ tcp::v4(), port_ }),
        users_(users) {}

Users& Server::GetUsers() {
    return users_;
}

void Server::do_accept() {
    acceptor_.async_accept([this](beast::error_code ec, tcp::socket socket) {
        if (!ec) {
            json::object obj;
            obj["address"] = socket.remote_endpoint().address().to_string();
            BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj)
                << logging::add_value("msg", "new connection"s);

            std::make_shared<Session>(std::move(socket), this)->Run();
        }
        do_accept();
        });
}

void Server::RunServer() {
    json::object obj;
    obj["port"] = port_;
    obj["address"] = acceptor_.local_endpoint().address().to_string();
    BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj)
        << logging::add_value("msg", "server is run"s);
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

void Server::RegisterSession(const std::string& user_id, std::shared_ptr<Session> session) {
    std::lock_guard lock(sessions_mutex_);
    sessions_[user_id] = session;
}

void Server::UnregisterSession(const std::string& user_id) {
    std::lock_guard lock(sessions_mutex_);
    sessions_.erase(user_id);
}

std::shared_ptr<Session> Server::FindSession(const std::string& user_id) {
    std::lock_guard lock(sessions_mutex_);
    auto it = sessions_.find(user_id);
    return (it != sessions_.end()) ? it->second : nullptr;
}

int main() {
    if (sodium_init() < 0) {
        std::cerr << "Libsodium not init\n";
        return 1;
    }
    InitLog();
    Users users;
    Server server(users);
    server.RunServer();
    return 0;
}