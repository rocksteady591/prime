#include "http_server.h"
#include <algorithm>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip//address.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/redis/src.hpp>
#include <boost/asio/ssl.hpp>
#include <exception>
#include <pqxx/internal/statement_parameters.hxx>
#include <stdexcept>
#include <memory>
#include <thread>
#include <iostream>
#include <vector>
#include <variant>
#include <cstdlib>
#include "log.h"
#include "connection_pool.h"
#include "user.h"
#include "chat.h"

namespace net = boost::asio;
namespace http = boost::beast::http;
namespace logging = boost::log;
namespace keywords = logging::keywords;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;
using pqxx::operator""_zv;

void InitLog() {
    logging::add_console_log(
        std::clog,
        keywords::format = &MyFormatter
    );
}

void CreateTables(pqxx::connection& sql){
    using namespace std::literals;
    json::object obj;
    try
    {
        pqxx::work txn(sql);
        constexpr auto create_index_username = "CREATE INDEX IF NOT EXISTS name_idx ON users (username);"_zv;
        constexpr auto create_index_login = "CREATE INDEX IF NOT EXISTS name_idx ON users (login);"_zv;
        constexpr auto create_index_find_chat =
            "CREATE INDEX IF NOT EXISTS us_pair_idx ON chats (LEAST(user1_id, user2_id), GREATEST(user1_id, user2_id));"_zv;
        constexpr auto create_index_chat_id = "CREATE INDEX IF NOT EXISTS messages_chat_id_idx ON messages(chat_id);"_zv;
        constexpr auto create_index_send_at = "CREATE INDEX IF NOT EXISTS messages_sand_at_idx ON messages(sender_id DESC);"_zv;
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS users(
                id SERIAL PRIMARY KEY,
                username VARCHAR(50) UNIQUE NOT NULL,
                login VARCHAR(50) UNIQUE NOT NULL,
                password_hash VARCHAR(255),
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
        )"_zv);
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS chats(
                id SERIAL PRIMARY KEY,
                user1_id integer REFERENCES users(id) NOT NULL,
                user2_id integer REFERENCES users(id) NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                UNIQUE(user1_id, user2_id)
            );
        )"_zv);
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS messages(
                id SERIAL PRIMARY KEY,
                chat_id integer REFERENCES chats(id) NOT NULL,
                sender_id integer REFERENCES users(id) NOT NULL,
                content text NOT NULL,
                sent_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
        )"_zv);
        txn.exec(create_index_chat_id);
        txn.exec(create_index_send_at);
        txn.exec(create_index_find_chat);
        txn.exec(create_index_username);
        txn.exec(create_index_login);
        txn.commit();
        obj["data"] = "createTable";
        obj["message"] = "Tables created seccessfully";
        BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj) << logging::add_value("msg", "Tables created seccessfully"s);
    }
    catch(const std::exception& e)
    {
        obj["error"] = "invalidCreateTables";
        obj["message"] = e.what();
        BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj) << logging::add_value("msg", "Tables not created seccessfully"s);
    }

}

Session::Session(tcp::socket&& socket, net::ssl::context& ctx, Users& users, ChatManager& chat_manager)
    : stream_(std::move(socket), ctx), handler_(RequestHandler(users, chat_manager)), users_(users) {
}

void Session::Run() {
    auto self = shared_from_this();
    stream_.async_handshake(net::ssl::stream_base::server, [self](beast::error_code ec){
            if(!ec){
                self->Read();
            }else{
                json::object obj;
                obj["Error"] = "sslHandshake";
                BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj)
                                         << logging::add_value("msg", ec.message());
            }
        });
}

void Session::Read() {
    http::async_read(stream_, buffer_, request_,
        beast::bind_front_handler(&Session::OnRead, this->shared_from_this()));
}

void Session::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_transfered) {
    if (ec) {
        json::object obj;
        obj["Error"] = "readingError";
        BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj) << logging::add_value("msg", ec.message());
        return;
    }
    HttpResponse response = handler_.HandleApiPost(std::move(request_));
    Write(std::move(response));
}

void Session::Write(HttpResponse response) {
    std::visit([this](auto&& arg) {
        http::async_write(stream_, *arg, [arg, self = shared_from_this()](beast::error_code ec, std::size_t bytes_transfered) {
            self->OnWrite(arg->need_eof(), ec, bytes_transfered);
            });
        }, response);

}

void Session::OnWrite(bool closed, beast::error_code ec, [[maybe_unused]] std::size_t bytes_transfered) {
    if (ec) {
        json::object obj;
        obj["Error"] = "writingError";
        BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj) << logging::add_value("msg", ec.message());
    }
    if (closed) {
        beast::error_code ecd;
        stream_.shutdown(ecd);
        if (ec) {
            json::object obj;
            obj["Error"] = "closeError";
            BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj) << logging::add_value("msg", ec.message());
        }
    }
    Read();
}

Listener::Listener(net::io_context& ioc, const tcp::endpoint& endpoint, net::ssl::context& ctx,
     Users& users, ChatManager& chat_manager)
    : ioc_(ioc), acceptor_(net::make_strand(ioc_)), ctx_(ctx), users_(users), chat_manager_(chat_manager) {
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(net::socket_base::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen(net::socket_base::max_listen_connections);
}

void Listener::RunServer() {
    DoAccept();
}

void Listener::DoAccept() {
    acceptor_.async_accept(ioc_.get_executor(), beast::bind_front_handler(&Listener::OnnAccept, this->shared_from_this()));
}

void Listener::OnnAccept(boost::system::error_code ec, tcp::socket socket) {
    if (ec) {
        json::object obj;
        obj["Error"] = "acceptError";
        BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj) << logging::add_value("msg", ec.message());
    }
    AsyncRunServer(std::move(socket));
    DoAccept();
}

void Listener::AsyncRunServer(tcp::socket socket) {
    std::make_shared<Session>(std::move(socket), ctx_, users_, chat_manager_)->Run();
}

template<typename T>
void RunWorkers(unsigned n, const T& fn) {
    std::vector<std::jthread> workers;
    int max = std::max(1u, n);
    workers.reserve(max - 1);
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

int main() {
    using namespace std::literals;
    constexpr net::ip::port_type port = 8081;
    InitLog();
    try {
        //take postgres connection url from environment variable
        const char* pg_db_path = std::getenv("PG_DB_URL");
        if(pg_db_path == nullptr){
            throw std::runtime_error("Postgres path is empty");
        }
        std::string pg_path(pg_db_path);
        const unsigned num_threads = std::thread::hardware_concurrency();
        //create connections pool
        ConnectionPool pool{num_threads, pg_path};
        {
            //initialize table and indecies
            auto wrapper = pool.GetConnection();
            pqxx::connection& connection = *wrapper;
            CreateTables(connection);
        }
        net::io_context ioc(num_threads);
        tcp::endpoint endpoint{ tcp::v4(), port };
        json::object obj;
        obj["port"] = port;
        obj["address"] = endpoint.address().to_string();
        BOOST_LOG_TRIVIAL(info) << logging::add_value("data", obj) << logging::add_value("msg", "server has started"s);
        Users users(pool);
        ssl::context ctx(ssl::context::tlsv12_server);
        ctx.set_options(
                ssl::context::default_workarounds |
                ssl::context::no_sslv2 |
                ssl::context::single_dh_use);
        ctx.use_certificate_file("server.crt", ssl::context::pem);
        ctx.use_private_key_file("server.key", ssl::context::pem);
        ctx.set_verify_mode(ssl::verify_none);

        ChatManager chat_manager(pool);
        std::make_shared<Listener>(ioc, endpoint, ctx, users, chat_manager)->RunServer();

        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
            });
    }
    catch (const std::exception& ex) {
        json::object obj;
        obj["Error"] = "dntStartServer";
        BOOST_LOG_TRIVIAL(error) << logging::add_value("data", obj) << logging::add_value("msg", ex.what());
    }
}
