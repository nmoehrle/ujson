#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <string_view>
#include <variant>
#include <memory>

namespace ujson
{
struct object;
struct array;
using value = std::variant<std::nullptr_t, std::unique_ptr<object>, std::unique_ptr<array>, double, int, bool>;

value read(std::string_view str);
void write(value val, std::string& str);
}

namespace ujson
{
}
