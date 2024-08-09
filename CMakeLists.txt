project(ujson)

add_library(ujson INTERFACE include/ujson.hpp)
target_include_directories(ujson INTERFACE include)

add_subdirectory(tests)
