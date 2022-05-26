cmake_minimum_required(VERSION 3.16)

set (CMAKE_CXX_FLAGS "-Wall -Wextra -Wconversion -Werror -O2")

project(client_bomberman)

set(CMAKE_CXX_STANDARD 20)
message("CMake flags: " ${CMAKE_CXX_FLAGS})

#add_library(constant constants.h)
add_library(connection_utils connection_utils.cpp)
add_library(input_utils input_utils.cpp)
add_executable(robots-client main.cpp) #-pthread -lboost_thread")

set(Boost_DEBUG 1)
find_package(Boost 1.71 COMPONENTS system iostreams date_time thread program_options REQUIRED)

target_link_libraries(robots-client connection_utils input_utils ${Boost_LIBRARIES})