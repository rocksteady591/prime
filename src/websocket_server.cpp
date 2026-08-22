#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/fields_fwd.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <exception>
#include <pqxx/pqxx>
#include <boost/json.hpp>
#include <cstddef>
#include <sodium.h>
#include <stdexcept>
#include <thread>
#include <iostream>
#include <iterator>
#include <vector>
#include <string>
#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <mutex>

#include "websocket_server.h"
#include "message.pb.h"
#include "log.h"
#include "user.h"
#include "connection_pool.h"
#include "chat.h"

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

Session::Session(tcp::socket&& socket, ssl::context& ctx,  Server* server)
    : ws_(std::move(socket), ctx), server_(server) {
}

Session::~Session() {
    if (!user_id_.empty()) {
        server_->UnregisterSession(user_id_);
    }
}

void Session::Run() {

    net::dispatch(ws_.get_executor(),
        beast::bind_front_handler(&Session::on_run, shared_from_this()));
    // Читаем HTTP-запрос апгрейда WebSocket
    /*beast::http::async_read(ws_.next_layer(),
                            buffer_,
                            upgrade_req_,
                            beast::bind_front_handler(&Session::on_read, shared_from_this()));*/
}

void Session::on_run(){
    ws_.next_layer().async_handshake(
        ssl::stream_base::server,
        beast::bind_front_handler(&Session::on_handshake, shared_from_this())
    );
}

void Session::on_handshake(beast::error_code ec){
    if(ec){
        json::object obj;
        obj["Error"] = "onHandshake";
        BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
            << logging::add_value("msg", "error ssl handshake" + ec.message());
        return;
    }

    beast::http::async_read(ws_.next_layer(),
                            buffer_,
                            upgrade_req_,
                            beast::bind_front_handler(&Session::on_read, shared_from_this()));

}

void Session::on_read(const beast::error_code& ec, std::size_t bytes_transfered){
    if(ec){
        json::object obj;
        obj["Error"] = "readUpgradeError";
        BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
            << logging::add_value("msg", "error reading" + ec.message());
        return;
    }

    std::string token;
    if(upgrade_req_.count(http::field::authorization) > 0){
        std::string auth_value(upgrade_req_[http::field::authorization]);
        if(!auth_value.starts_with("Bearer")){
            json::object obj;
            obj["Error"] = "readUpgradeError";
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                << logging::add_value("msg", "Empty or incorrect token");
            return;
        }

        try {
            size_t start = 7;
            token = auth_value.substr(start, auth_value.size() - start);
        } catch (const std::exception& e) {
        json::object obj;
            obj["Error"] = "parsingError";
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                << logging::add_value("msg", e.what());
            return;
        }

        // Проверяем токен
        User* user = server_->GetUsers().FindUserByToken(token);
        if (!user) {
            beast::error_code ec;
            json::object obj;
            obj["Warning"] = "invalidToken";
            BOOST_LOG_TRIVIAL(warning) << logging::add_value("data", obj)
                << logging::add_value("msg", "Invalid token");
            // Отправляем 401 и закрываем соединение
            beast::http::response<beast::http::string_body> res{
                beast::http::status::unauthorized, upgrade_req_.version() };
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
        ws_.async_accept(upgrade_req_, [self = shared_from_this()](beast::error_code ec) {
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

ChatManager& Server::GetManager(){
    return chat_manager_;
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
        std::string message(plaintext.begin(), plaintext.end());
        int sender = std::stoi(sender_id);
        int recip = std::stoi(recipient_id);
        if(sender > recip){
            std::swap(sender, recip);
        }
        //тут создается новый или возвращается уже существующий чат
        //принимает айди отправителя и получателя
        int chat_id = self->server_->GetManager().CreateOrGetChat(sender, recip);
        self->server_->GetManager().AddMessage(std::stoi(sender_id), chat_id, message);
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

Server::Server(Users& users, ChatManager& chat_manager)
    :   threads_count_(std::thread::hardware_concurrency()),
        io_context_(threads_count_),
        acceptor_(io_context_, tcp::endpoint{ tcp::v4(), port_ }),
        users_(users),
        chat_manager_(chat_manager){
            //для самоподписных сертификатов
            ctx_.set_options(
                ssl::context::default_workarounds |
                ssl::context::no_sslv2 |
                ssl::context::single_dh_use);
            ctx_.use_certificate_file("/Users/philingosling/Documents/primal/server.crt", ssl::context::pem);
            ctx_.use_private_key_file("/Users/philingosling/Documents/primal/server.key", ssl::context::pem);
            //отклбчаем проверку
            ctx_.set_verify_mode(ssl::verify_none);
        }

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

            std::make_shared<Session>(std::move(socket), ctx_, this)->Run();
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
    try{
        const char* pg_db_path = std::getenv("PG_DB_URL");
        if(pg_db_path == nullptr){
            throw std::runtime_error("Postgres path is empty");
        }
        std::string pg_path(pg_db_path);
        ConnectionPool pool{std::thread::hardware_concurrency(), pg_path};
        InitLog();
        Users users(pool);

        ChatManager chat{pool};
        Server server(users, chat);
        server.RunServer();
    }catch(const std::exception& e){
        json::object obj;
        obj["error"] = "Server dont run";
        obj["message"] = e.what();
        BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj)
            << logging::add_value("msg", "server dont run"s);
    }

    return 0;
}
