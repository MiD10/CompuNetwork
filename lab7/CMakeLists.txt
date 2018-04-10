cmake_minimum_required(VERSION 2.8)
project(lab7)

include_directories("${PROJECT_SOURCE_DIR}")

SET(CMAKE_CXX_COMPILER "g++")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -march=native -O3 -pthread")

add_executable(server server.cpp client_server.h)
add_executable(client client.cpp client_server.h)