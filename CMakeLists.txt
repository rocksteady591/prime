cmake_minimum_required(VERSION 3.25)
project(Primal CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost REQUIRED COMPONENTS system CONFIG)
find_package(libsodium REQUIRED)
find_package(protobuf REQUIRED)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS message.proto)

add_executable(primal_server src/server.cpp src/server.h src/key_handler.h ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(http_server src/http_server.cpp src/http_server.h)
#add_executable(primal_client src/client.cpp src/user.h
    #src/client.h ${PROTO_SRCS} ${PROTO_HDRS})
    # Указываем, что хедеры лежат там, куда их положил CMake при генерации
target_include_directories(primal_server PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(http_server PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
#target_include_directories(primal_client PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(http_server PRIVATE Boost::system Boost::json Boost::log Boost::thread)
target_link_libraries(primal_server PRIVATE Boost::system Boost::json Boost::log Boost::thread)
target_link_libraries(primal_server PRIVATE libsodium::libsodium)
target_link_libraries(primal_server PRIVATE protobuf::libprotobuf)
#target_link_libraries(primal_client PRIVATE Boost::system Boost::json Boost::log Boost::thread)
#target_link_libraries(primal_client PRIVATE libsodium::libsodium)
#target_link_libraries(primal_client PRIVATE protobuf::libprotobuf)
