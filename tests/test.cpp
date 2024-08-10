#include <ujson.hpp>

#include <iostream>

int main()
{
    auto obj = ujson::object();
    obj["s"] = "str";
    obj["e"] = "\\\"str";
    obj["i"] = 1337;
    obj["d"] = 3.14;
    obj["t"] = true;
    obj["f"] = false;
    obj["0"] = nullptr;
    obj["arr"] = ujson::array({42, 2.71, "foo", true, false, nullptr});
    const auto val0 = ujson::value(obj);
    const auto str = ujson::serialize(val0);
    const auto val1 = ujson::parse(str);
    std::cout << str << std::endl;
    if (val0 != val1)
    {
        exit(EXIT_FAILURE);
    }
}
