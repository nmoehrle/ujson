add_executable(ujson_test test.cpp)
target_link_libraries(ujson_test PRIVATE ujson)

add_test(NAME ujson_test COMMAND ujson_test)
