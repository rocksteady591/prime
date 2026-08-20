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

Session::Session(tcp::socket socket, Server* server)
    : ws_(std::move(socket)), server_(server) {
}

Session::~Session() {
    BOOST_LOG_TRIVIAL(info) << logging::add_value("data", json::object{{"user_id", user_id_}})
                            << logging::add_value("msg", "Session destroyed");
    // Принудительно закрываем сокет (без удаления из реестра – это делается явно)
    beast::error_code ec;
    ws_.close(websocket::close_code::normal, ec);
    ws_.next_layer().shutdown(tcp::socket::shutdown_both, ec);
    ws_.next_layer().close(ec);
}

// ----- НОВЫЙ МЕТОД: принудительное закрытие сокета без удаления сессии -----
void Session::ForceClose() {
    beast::error_code ec;
    ws_.close(websocket::close_code::normal, ec);
    ws_.next_layer().shutdown(tcp::socket::shutdown_both, ec);
    ws_.next_layer().close(ec);
}

void Session::Run() {
    try {
        // 1. Очищаем буфер (на всякий случай)
        buffer_.consume(buffer_.size());

        // 2. Читаем HTTP‑запрос вручную
        beast::http::request<beast::http::string_body> upgrade_req;
        beast::error_code ec;
        http::read(ws_.next_layer(), buffer_, upgrade_req, ec);
        if (ec) {
            json::object obj;
            obj["Error"] = "readUpgradeError";
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                                     << logging::add_value("msg", ec.message());
            return;
        }

        // 3. Логируем метод и target
        json::object req_log;
        req_log["method"] = upgrade_req.method_string();
        req_log["target"] = upgrade_req.target();
        req_log["version"] = upgrade_req.version();
        BOOST_LOG_TRIVIAL(info) << logging::add_value("data", req_log)
                                << logging::add_value("msg", "HTTP request received");

        // 4. Извлекаем токен
        std::string target = upgrade_req.target();
        std::string token;
        auto pos = target.find("?token=");
        if (pos != std::string::npos) {
            token = target.substr(pos + 7);
            auto end = token.find_first_of("&#");
            if (end != std::string::npos) token = token.substr(0, end);
            token.erase(std::remove_if(token.begin(), token.end(), ::isspace), token.end());
        }

        json::object tok;
        tok["token"] = token;
        BOOST_LOG_TRIVIAL(info) << logging::add_value("data", tok)
                                << logging::add_value("msg", "Token extracted");

        // Проверяем пользователя
        User* user = nullptr;
        try {
            user = server_->GetUsers().FindUserByToken(token);
        } catch (const std::exception& e) {
            json::object obj;
            obj["Error"] = "findUserException";
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                                     << logging::add_value("msg", e.what());
            ws_.close(websocket::close_code::policy_error);
            return;
        }
        if (!user) {
            json::object err;
            err["reason"] = "invalid_token";
            BOOST_LOG_TRIVIAL(warning) << logging::add_value("data", err)
                                       << logging::add_value("msg", "User not found for token");
            ws_.close(websocket::close_code::policy_error);
            return;
        }

        // Регистрируем сессию (старая сессия будет удалена внутри RegisterSession)
        user_id_ = std::to_string(user->GetId());
        server_->RegisterSession(user_id_, shared_from_this());  // <-- теперь RegisterSession сам удаляет старую

        json::object reg;
        reg["user_id"] = user_id_;
        BOOST_LOG_TRIVIAL(info) << logging::add_value("data", reg)
                                << logging::add_value("msg", "Session registered");

        // 5. Очищаем буфер (данные уже в upgrade_req)
        buffer_.consume(buffer_.size());

        // 6. Выполняем WebSocket handshake, передавая прочитанный запрос
        ws_.accept(upgrade_req, ec);
        if (ec) {
            json::object obj;
            obj["Error"] = "acceptError";
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                                     << logging::add_value("msg", ec.message());
            // Если handshake не удался – удаляем сессию из реестра
            if (!user_id_.empty()) {
                auto current = server_->FindSession(user_id_);
                if (current && current.get() == this) {
                    server_->UnregisterSession(user_id_);
                }
            }
            return;
        }

        // Начинаем обмен данными
        ws_.binary(true);
        DoRead();
    } catch (const std::exception& e) {
        json::object obj;
        obj["exception"] = e.what();
        BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                                 << logging::add_value("msg", "Unhandled exception in Run");
        beast::error_code ec;
        ws_.close(websocket::close_code::internal_error, ec);
        // Если произошло исключение после регистрации – удаляем сессию
        if (!user_id_.empty()) {
            auto current = server_->FindSession(user_id_);
            if (current && current.get() == this) {
                server_->UnregisterSession(user_id_);
            }
        }
    }
}

void Session::key_exchange(const std::vector<unsigned char>& received_key) {
    auto [pk, sk] = generate_keypair();
    sk_ = std::move(sk);
    shared_secret_key_ = compute_shared_key(sk_, received_key);

    auto self = shared_from_this();
    ws_.async_write(
        net::buffer(pk.data(), pk.size()),
        [self](beast::error_code ec, std::size_t /*bytes*/) {
            if (ec) {
                json::object obj;
                obj["Error"] = "pb_keyDontSend";
                BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                    << logging::add_value("msg", ec.message());
                return;
            }
            json::object obj;
            obj["Info"] = "publicKeySent";
            BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj)
                << logging::add_value("msg", "Public key sent, calling DoRead");
            self->DoRead();
        });
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
    {
        json::object obj;
        obj["shared_key_exists"] = !shared_secret_key_.empty();
        BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj)
                                << logging::add_value("msg", "DoRead called");
    }
    ws_.async_read(buffer_, [self = shared_from_this()](beast::error_code ec, std::size_t bytes_read) {
        if (ec) {
            json::object obj;
            obj["Error"] = "error read";
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                                    << logging::add_value("msg", ec.message());
            // Проверяем, что мы всё ещё зарегистрированы и являемся активной сессией
            auto current = self->server_->FindSession(self->user_id_);
            if (current && current.get() == self.get()) {
                self->server_->UnregisterSession(self->user_id_);
            }
            return;
        }
        {
            json::object obj;
            obj["bytes_read"] = static_cast<std::int64_t>(bytes_read);
            BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj)
                                    << logging::add_value("msg", "Received bytes");
        }

        // Первый шаг: обмен ключами
        if (self->shared_secret_key_.empty()) {
            std::vector<unsigned char> received_key(bytes_read);
            net::buffer_copy(net::buffer(received_key.data(), received_key.size()),
                self->buffer_.data());
            self->key_exchange(received_key);
            self->buffer_.consume(bytes_read);
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
            self->DoRead();
            return;
        }

        const std::string& ciphertext = recieve_envelope.ciphertext();
        const std::string& nonce = recieve_envelope.nonce();
        std::string sender_id = recieve_envelope.sender_id();
        std::string recipient_id = recieve_envelope.recipient_id();

        if (sender_id.empty() || recipient_id.empty()) {
            json::object obj;
            obj["Warning"] = "emptyId";
            BOOST_LOG_TRIVIAL(warning) << logging::add_value("data", obj)
                                    << logging::add_value("msg", "sender_id or recipient_id is empty");
            self->DoRead();
            return;
        }

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

        int sender = 0, recip = 0;
        try {
            sender = std::stoi(sender_id);
            recip = std::stoi(recipient_id);
        } catch (const std::exception& e) {
            json::object obj;
            obj["Error"] = "invalidId";
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                                    << logging::add_value("msg", e.what());
            self->DoRead();
            return;
        }

        if(sender > recip){
            std::swap(sender, recip);
        }
        int chat_id = self->server_->GetManager().CreateOrGetChat(sender, recip);
        self->server_->GetManager().AddMessage(std::stoi(sender_id), chat_id, message);

        if (!recipient_id.empty()) {
            auto target = self->server_->FindSession(recipient_id);
            if (target) {
                std::vector<unsigned char> new_nonce(crypto_box_NONCEBYTES);
                randombytes_buf(new_nonce.data(), new_nonce.size());

                std::vector<unsigned char> encrypted(message.size() + crypto_box_MACBYTES);
                crypto_box_easy_afternm(
                    encrypted.data(),
                    reinterpret_cast<const unsigned char*>(message.data()),
                    message.size(),
                    new_nonce.data(),
                    target->shared_secret_key_.data());

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
                json::object obj;
                obj["recipient"] = recipient_id;
                BOOST_LOG_TRIVIAL(warning) << logging::add_value("data", obj)
                    << logging::add_value("msg", "Recipient offline");
            }
        }

        self->DoRead();
    });
}

// ----- КЛАСС Server -----
Server::Server(Users& users, ChatManager& chat_manager)
    :   threads_count_(std::thread::hardware_concurrency()),
        io_context_(threads_count_),
        acceptor_(io_context_, tcp::endpoint{ tcp::v4(), port_ }),
        users_(users),
        chat_manager_(chat_manager) {}

Users& Server::GetUsers() {
    return users_;
}

void Server::do_accept() {
    json::object obj;
    obj["action"] = "accept_wait";
    BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj)
                            << logging::add_value("msg", "Waiting for connection"s);

    acceptor_.async_accept([this](beast::error_code ec, tcp::socket socket) {
        if (ec) {
            json::object err;
            err["error"] = ec.message();
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", err)
                                     << logging::add_value("msg", "Accept failed"s);
        } else {
            json::object ok;
            ok["remote"] = socket.remote_endpoint().address().to_string();
            BOOST_LOG_TRIVIAL(info) << logging::add_value("data", ok)
                                    << logging::add_value("msg", "New connection accepted"s);
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
    auto it = sessions_.find(user_id);
    if (it != sessions_.end()) {
        json::object obj;
        obj["user_id"] = user_id;
        BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj)
                                << logging::add_value("msg", "Erasing old session");
        // Сначала удаляем старую сессию из реестра
        auto old_session = it->second;
        sessions_.erase(it);
        // Затем закрываем сокет старой сессии (она больше не в реестре)
        old_session->ForceClose();
    }
    sessions_[user_id] = session;
    BOOST_LOG_TRIVIAL(info) << logging::add_value("data", json::object{{"user_id", user_id}})
                        << logging::add_value("msg", "Session inserted");
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