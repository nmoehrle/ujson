#include <ujson.hpp>

#include <iostream>
#include <sstream>

int main()
{
    std::stringstream ss;
    ss << std::cin.rdbuf();
    std::cout << ujson::write(ujson::read(ss.str()));
}
