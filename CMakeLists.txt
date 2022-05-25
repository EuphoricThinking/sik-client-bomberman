cmake_minimum_required(VERSION 3.16)
project(client_bomberman)

set(CMAKE_CXX_STANDARD 20)

#add_library(constant constants.h)
add_library(connection_utils connection_utils.cpp)
add_executable(robots-client main.cpp)
set (CMAKE_CXX_FLAGS "-Wall -Wextra -Wconversion -Werror -O2") #-pthread -lboost_thread")

set(Boost_DEBUG 1)
find_package(Boost 1.71 COMPONENTS system iostreams date_time thread program_options REQUIRED)

target_link_libraries(robots-client connection_utils ${Boost_LIBRARIES})