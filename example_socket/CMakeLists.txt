cmake_minimum_required(VERSION 2.8)
project(lab7)

SET(CMAKE_CXX_COMPILER "g++")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -march=native -O3 -pthread")

add_executable(server example_server.c)
add_executable(client example_client.c)
