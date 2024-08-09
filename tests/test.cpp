#include <ujson.hpp>

#include <iostream>

int main()
{
    auto obj = ujson::object();
    obj["s"] = std::string("str");
    obj["i"] = 1337;
    obj["d"] = 3.14;
    obj["t"] = true;
    obj["f"] = false;
    obj["0"] = nullptr;
    obj["arr"] = ujson::array({42, 2.71, "foo", true, false, nullptr});
    auto val = ujson::value(obj);
    const auto str = ujson::serialize(val);
    std::cout << str << std::endl;
    if (val != ujson::parse(str))
    {
        exit(EXIT_FAILURE);
    }
}
