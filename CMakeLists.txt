cmake_minimum_required(VERSION 3.22)
project(client_bomberman)

set(CMAKE_CXX_STANDARD 20)

add_executable(client_bomberman main.cpp)
set (CMAKE_CXX_FLAGS "-Wall -Wextra -Wconversion -Werror -O2") #-pthread -lboost_thread")

set(Boost_DEBUG 1)
find_package(Boost 1.71 COMPONENTS system iostreams date_time thread REQUIRED)

target_link_libraries(client_bomberman ${Boost_LIBRARIES})
