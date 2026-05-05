#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core.hpp>
#include <iostream>
#include <thread>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
using tcp = net::ip::tcp;
using namespace std::literals;

int main(){
    std::string user_input;
    beast::flat_buffer buffer;
    try{
        net::io_context io_context;
        tcp::resolver resolver{io_context};
        websocket::stream<tcp::socket> ws{io_context};

        auto const result = resolver.resolve("127.0.0.1", "8080");
        net::connect(ws.next_layer(), result.begin(), result.end());
        ws.handshake("127.0.0.1", "/");
        while(true){
            buffer.clear();
            beast::error_code ec;
            std::cout << "ВВедите сообщение: ";
            std::getline(std::cin, user_input);
            if(user_input == "exit"){
                break;
            }
            ws.write(net::buffer(user_input), ec);
            if(ec){
                std::cerr << "Error sending data" << std::endl;
            }
            ws.read(buffer, ec);
            if(ec){
            std::cerr << "Error reading data" << std::endl;
            }else{
                std::cout << "Ответ от сервера: " << beast::buffers_to_string(buffer.data()) << std::endl;
            }
        }
        
        
        ws.close(websocket::close_code::normal);
    }catch(const std::exception& e){
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}