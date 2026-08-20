cmake_minimum_required(VERSION 3.15)
project(TEST CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A00)
endif()

set(CMAKE_CXX_STANDARD 23)
set(WITH_POSTGRESQL ON CACHE INTERNAL "")

# Ищем пакеты, предоставленные Conan
find_package(Boost REQUIRED COMPONENTS json log thread)
find_package(libsodium REQUIRED)
find_package(protobuf REQUIRED)
find_package(libpqxx REQUIRED)
find_package(OpenSSL REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS public/proto/message.proto)

add_library(proto_objects OBJECT ${PROTO_SRCS} ${PROTO_HDRS})

target_include_directories(proto_objects PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(proto_objects PRIVATE protobuf::libprotobuf)

add_library(common_objects OBJECT
    "src/user.cpp"
    "src/request_handler.cpp"
    "src/request_handler.h"
    "src/connection_pool.h"
    "src/chat.cpp"
    "src/chat.h"
        src/chat.cpp
)

target_include_directories(common_objects PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(common_objects
    PRIVATE
    Boost::json
    Boost::log
    Boost::thread
    libsodium::libsodium
    protobuf::libprotobuf
    libpqxx::pqxx
    OpenSSL::Crypto
)

# --- HTTP_SERVER ---
add_executable(http_server
    "src/http_server.cpp"
    "src/http_server.h"
    "src/log.h"
    $<TARGET_OBJECTS:proto_objects>
    $<TARGET_OBJECTS:common_objects>
)

target_include_directories(http_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(http_server
    PRIVATE
    Boost::json
    Boost::log
    Boost::thread
    libsodium::libsodium
    protobuf::libprotobuf
    libpqxx::pqxx
    OpenSSL::SSL
    OpenSSL::Crypto
)

# --- WEBSOCKET_SERVER ---
add_executable(websocket_server
    "src/websocket_server.cpp"
    "src/websocket_server.h"
    "src/log.h"
    $<TARGET_OBJECTS:proto_objects>
    $<TARGET_OBJECTS:common_objects>
)

target_include_directories(websocket_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(websocket_server
    PRIVATE
    Boost::json
    Boost::log
    Boost::thread
    libsodium::libsodium
    protobuf::libprotobuf
    libpqxx::pqxx
    OpenSSL::SSL
    OpenSSL::Crypto
)
