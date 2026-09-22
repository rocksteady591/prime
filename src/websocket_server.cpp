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
#include <sstream>

#include "websocket_server.h"
#include "message.pb.h"
#include "log.h"
#include "user.h"
#include "connection_pool.h"
#include "chat.h"
#include "jwt_utils.h"

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
namespace logging = boost::log;
namespace keywords = logging::keywords;
namespace json = boost::json;
using tcp = net::ip::tcp;
using namespace std::literals;

std::string extract_token_from_request(const beast::http::request<beast::http::string_body>& req) {
    // 1. Проверяем заголовок Authorization (для обычных HTTP-запросов и WebSocket с кастомными заголовками, если они поддерживаются)
    auto it_auth = req.find(http::field::authorization);
    if (it_auth != req.end()) {
        std::string auth = it_auth->value();
        if (auth.size() > 7 && auth.substr(0, 7) == "Bearer ") {
            return auth.substr(7);
        }
    }

    // 2. Проверяем Sec-WebSocket-Protocol (основной способ для браузерных WebSocket)
    auto it_proto = req.find("Sec-WebSocket-Protocol");
    if (it_proto != req.end()) {
        std::string token = it_proto->value();
        // Удаляем пробелы и лишние символы
        token.erase(std::remove_if(token.begin(), token.end(), ::isspace), token.end());
        if (!token.empty()) {
            return token;
        }
    }

    // 3. Fallback: проверяем URL-параметр (небезопасно, но для совместимости)
    auto target = req.target();
    auto pos = target.find('?');
    if (pos != std::string::npos) {
        std::string query = target.substr(pos + 1);
        std::stringstream ss(query);
        std::string item;
        while (std::getline(ss, item, '&')) {
            auto eq = item.find('=');
            if (eq != std::string::npos && item.substr(0, eq) == "token") {
                return item.substr(eq + 1);
            }
        }
    }
    return {};
}

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
    if (crypto_box_beforenm(shared_secret_key, other_pk.data(), my_sk.data()) != 0) {
        throw std::runtime_error("Failed to compute shared key");
    }
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

net::io_context& Server::GetContext(){
    return io_context_;
}

Session::Session(tcp::socket&& socket, ssl::context& ctx,  Server* server)
    : ws_(std::move(socket), ctx), server_(server), strand_(net::make_strand(server->GetContext())) {
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

void Session::on_read(const beast::error_code& ec, std::size_t bytes_transfered) {
    if (ec) {
        json::object obj;
        obj["Error"] = "readUpgradeError";
        BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
            << logging::add_value("msg", "error reading" + ec.message());
        return;
    }

    std::string token = extract_token_from_request(upgrade_req_);
    std::cout << "Extracted token: " << token << std::endl;

    if (token.empty()) {
        // Нет токена → 401
        beast::http::response<beast::http::string_body> res{
            beast::http::status::unauthorized, upgrade_req_.version() };
        res.set(http::field::server, "Messenger");
        res.set(http::field::content_type, "text/plain");
        res.body() = "Missing token";
        res.prepare_payload();
        beast::error_code ec_write;
        beast::http::write(ws_.next_layer(), res, ec_write);
        if (ec_write) {
            json::object obj;
            obj["Error"] = "write401";
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                << logging::add_value("msg", "Failed to write 401 response: " + ec_write.message());

        }
        return;
    }

    // Проверяем JWT
    std::string user_id_str;
    if (!VerifyJWT(token, JWT_SECRET_KEY, user_id_str)) {
        // Невалидный токен → 401
        beast::http::response<beast::http::string_body> res{
            beast::http::status::unauthorized, upgrade_req_.version() };
        res.set(http::field::server, "Messenger");
        res.set(http::field::content_type, "text/plain");
        res.body() = "Invalid token";
        res.prepare_payload();
        beast::error_code ec_write;
        beast::http::write(ws_.next_layer(), res, ec_write);
        if (ec_write) {
            json::object obj;
            obj["Error"] = "write401";
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                << logging::add_value("msg", "Failed to write 401 response: " + ec_write.message());
        }
        return;
    }

    int user_id = std::stoi(user_id_str);
    User* user = server_->GetUsers().FindUserById(user_id);
    if (!user) {
        // Пользователь не найден → 401
        beast::http::response<beast::http::string_body> res{
            beast::http::status::unauthorized, upgrade_req_.version() };
        res.set(http::field::server, "Messenger");
        res.set(http::field::content_type, "text/plain");
        res.body() = "User not found";
        res.prepare_payload();
        beast::error_code ec_write;
        beast::http::write(ws_.next_layer(), res, ec_write);
        if (ec_write) {
            json::object obj;
            obj["Error"] = "write401";
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                << logging::add_value("msg", "Failed to write 401 response: " + ec_write.message());
        }
        return;
    }
    // Сохраняем user_id и регистрируем сессию
    user_id_ = std::to_string(user_id);
    server_->RegisterSession(user_id_, shared_from_this());

    // Устанавливаем декоратор для ответа, чтобы вернуть Sec-WebSocket-Protocol,
    // если клиент его передал
    auto it_proto = upgrade_req_.find("Sec-WebSocket-Protocol");
    if (it_proto != upgrade_req_.end()) {
        std::string protocol = it_proto->value();
        // Можно взять первый протокол из списка (разделённых запятыми)
        // Для простоты берём как есть
        ws_.set_option(websocket::stream_base::decorator(
            [protocol](websocket::response_type& res) {
                res.set("Sec-WebSocket-Protocol", protocol);
            }
        ));
    }

    // Выполняем WebSocket handshake
    ws_.async_accept(upgrade_req_,
    [self = shared_from_this()](beast::error_code ec_accept) {
        if (ec_accept) {
            json::object obj;
            obj["Error"] = "acceptError";
            obj["code"] = ec_accept.value();
            obj["category"] = ec_accept.category().name();
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                << logging::add_value("msg", "WebSocket accept failed: " + ec_accept.message());
            return;
        }
        json::object obj;
        obj["status"] = "accepted";
        BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj)
            << logging::add_value("msg", "Accept succeeded");
        self->ws_.binary(true);
        self->buffer_.consume(self->buffer_.size());
        self->DoRead();
    });
}

void Session::key_exchange(const std::vector<unsigned char>& received_key) {
    auto [pk, sk] = generate_keypair();
    auto shared_pk = std::make_shared<std::vector<unsigned char>>(std::move(pk));
    sk_ = std::move(sk);
    shared_secret_key_ = compute_shared_key(sk_, received_key);

    json::object obj_log;
    obj_log["action"] = "sending_public_key";
    BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj_log)
                            << logging::add_value("msg", "Sending public key to client...");

    // Отправляем ключ, и только после успешной отправки начинаем новое чтение
    ws_.async_write(
        net::buffer(shared_pk->data(), shared_pk->size()),
        [self = shared_from_this(), shared_pk](beast::error_code ec, std::size_t bytes) {
            if (ec) {
                json::object obj;
                obj["Error"] = "pb_keyDontSend";
                BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                                         << logging::add_value("msg", ec.message());
                return;
            }
            json::object obj;
            obj["status"] = "public_key_sent";
            BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj)
                                    << logging::add_value("msg", "Public key sent, " + std::to_string(bytes) + " bytes");
            self->SendOfflineMessages();
            // Теперь начинаем чтение
            self->DoRead();
        });
}

void Session::SendRaw(const std::string& raw_data) {
    auto sp = std::make_shared<std::string>(raw_data);

    //add post
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
        json::object obj_bytes;
        obj_bytes["bytes"] = bytes_read;
        BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj_bytes)
            << logging::add_value("msg", "Received bytes from client");

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
        try {
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
        } catch (...) {
            self->DoRead();
            return;
        }
        });
}

void Session::Close() {
    beast::error_code ec;
    ws_.close(websocket::close_code::normal, ec);
    user_id_.clear();
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
                ssl::context::single_dh_use |
                ssl::context::no_tlsv1 |
                ssl::context::no_tlsv1_1
            );
            ctx_.set_verify_mode(ssl::verify_none);
            const char* cert_file = std::getenv("SERVER_CERT_FILE");
            const char* key_file = std::getenv("SERVER_KEY_FILE");
            if (!cert_file || !key_file) {
                throw std::runtime_error("Missing SSL certificate environment variables");
            }
            ctx_.use_certificate_file(cert_file, ssl::context::pem);
            ctx_.use_private_key_file(key_file, ssl::context::pem);
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
    auto it = sessions_.find(user_id);
    if (it != sessions_.end()) {
        // При обновлении страницы старое соединение может уничтожиться уже
        // после регистрации нового и не должно удалить новую запись.
        it->second->Close();
        sessions_.erase(it);
    }
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

void Session::SendOfflineMessages() {
    if (user_id_.empty()) return;
    int user_id = std::stoi(user_id_);
    // Получаем все недоставленные сообщения для этого пользователя
    auto msgs = server_->GetManager().GetUndeliveredMessages(user_id);
    if (msgs.empty()) {
        BOOST_LOG_TRIVIAL(info) << "No undelivered messages for user " << user_id;
        return;
    }
    BOOST_LOG_TRIVIAL(info) << "Sending " << msgs.size() << " undelivered messages to user " << user_id;

    for (const auto& msg : msgs) {
        // Получаем отправителя (может быть не в сети, но это не важно)
        std::string sender_id = std::to_string(msg.sender_id);
        std::string recipient_id = std::to_string(user_id);

        // Шифруем сообщение для текущего пользователя (ключ уже есть)
        std::vector<unsigned char> nonce(crypto_box_NONCEBYTES);
        randombytes_buf(nonce.data(), nonce.size());

        std::vector<unsigned char> encrypted(msg.text.size() + crypto_box_MACBYTES);
        crypto_box_easy_afternm(
            encrypted.data(),
            reinterpret_cast<const unsigned char*>(msg.text.data()),
            msg.text.size(),
            nonce.data(),
            shared_secret_key_.data());   // текущий shared key (клиента)

        messenger::SecureEnvelope env;
        env.set_ciphertext(encrypted.data(), encrypted.size());
        env.set_nonce(nonce.data(), nonce.size());
        env.set_sender_id(sender_id);
        env.set_recipient_id(recipient_id);

        std::string serialized;
        env.SerializeToString(&serialized);
        SendRaw(serialized);

        // Помечаем сообщение как доставленное
        server_->GetManager().MarkMessageDelivered(msg.id);
    }
}
