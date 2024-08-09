cmake_minimum_required(VERSION 3.20)

project(ujson)

set(CMAKE_CXX_STANDARD 20)

add_library(ujson INTERFACE include/ujson.hpp)
target_include_directories(ujson INTERFACE include)

add_subdirectory(tests)
