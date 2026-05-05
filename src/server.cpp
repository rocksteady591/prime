#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <thread>
#include <iostream>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
using tcp = net::ip::tcp;
using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;
using namespace std::literals;

void RequestHandler(websocket::stream<tcp::socket>& ws){
    StringRequest req;
    beast::flat_buffer buffer;
    beast::error_code ec;
    http::read(ws, buffer, req, ec);
    std::cout << req.method_string() << ' ' << req.target() << std::endl;
    for(const auto& header : req){
        std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
    }
}

int main(){
    try{
        const auto adress = net::ip::make_address("127.0.0.1");
        constexpr uint16_t port = 8080;
        net::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint{tcp::v4(), port});
        std::cout << "Server has started..." << std::endl;
        
        for(;;){
            tcp::socket socket{io_context};
            acceptor.accept(socket);    
            websocket::stream<tcp::socket> ws{std::move(socket)};
            ws.accept();
            std::thread t([](websocket::stream<tcp::socket> ws){
                beast::flat_buffer buffer;
                for(;;){

                    beast::error_code ec;
                    ws.read(buffer, ec);
                    if (ec) {
                        if (ec == websocket::error::closed) {
                            std::cout << "Сессия завершена: клиент закрыл соединение." << std::endl;
                        } else {
                            std::cerr << "Error reading data: " << ec.message() << std::endl;
                        }
                        break;
                    }
                    std::cout << "Сервер получил: " << beast::buffers_to_string(buffer.data()) << std::endl;

                    ws.write(net::buffer(buffer.data()), ec);
                    if(ec){
                        std::cerr << "Error sending data" << std::endl;
                        break;
                    }
                    buffer.clear();
                }
            }, std::move(ws));
            
            t.detach();
            //RequestHandler(ws);
            
        }
    }catch(const beast::system_error& se){
        std::cerr << "Ошибка: " << se.what() << std::endl;
        return EXIT_FAILURE;
    }
    
}