cmake_minimum_required(VERSION 3.25)
project(Primal CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED COMPONENTS system CONFIG)

add_executable(primal_server src/server.cpp src/server.h src/user.cpp src/user.h)
add_executable(primal_client src/client.cpp src/user.cpp src/user.h
    src/client.h)
target_link_libraries(primal_server PRIVATE Boost::system)
target_link_libraries(primal_client PRIVATE Boost::system)