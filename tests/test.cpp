#include <ujson.hpp>

#include <iostream>
#include <sstream>

int main()
{
    std::stringstream ss;
    ss << std::cin.rdbuf();
    const auto json = ujson::read(ss.str());
    std::cout << write(json);
}
