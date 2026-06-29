cmake_minimum_required(VERSION 3.15)
project(TEST CXX)
if(WIN32)
    # 0x0A00 означает Windows 10 / Windows 11 / Windows Server 2016+
    add_compile_definitions(_WIN32_WINNT=0x0A00)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_GENERATOR_PLATFORM x64)
set(WITH_POSTGRESQL ON CACHE INTERNAL "")


# Ищем пакеты, предоставленные Conan
find_package(Boost REQUIRED COMPONENTS json log thread)
find_package(libsodium REQUIRED)
find_package(protobuf REQUIRED)
find_package(libpqxx REQUIRED)


# Используем классический макрос (он автоматически подхватит protoc из Conan)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS message.proto)

# Добавляем сгенерированные файлы в исполняемый файл
add_executable(http_server 
     "src/http_server.cpp" 
     "src/http_server.h"
    ${PROTO_SRCS} 
    ${PROTO_HDRS}
  "src/request_handler.h" "src/request_handler.cpp" "src/log.h" "src/user.cpp" "src/user.h")
add_executable(websocket_server
    "src/websocket_server.cpp"
    "src/websocket_server.h"
    ${PROTO_SRCS}
    ${PROTO_HDRS} "src/log.h" "src/user.cpp" "src/user.h")

# Указываем, где искать сгенерированный message.pb.h
target_include_directories(http_server PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(websocket_server PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(http_server 
    PRIVATE 
    Boost::json 
    Boost::log 
    Boost::thread 
    libsodium::libsodium
    protobuf::libprotobuf
    libpqxx::pqxx
)
target_link_libraries(websocket_server 
    PRIVATE 
    Boost::json 
    Boost::log 
    Boost::thread 
    libsodium::libsodium 
    protobuf::libprotobuf
    libpqxx::pqxx
)

