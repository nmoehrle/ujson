#include <ujson.hpp>

#include <iostream>
#include <sstream>

int main()
{
    std::stringstream ss;
    ss << std::cin.rdbuf();
    std::cout << ujson::serialize(ujson::parse(ss.str()));
}
