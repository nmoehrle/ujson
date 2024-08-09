#pragma once

#include <charconv>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace ujson
{
struct object;
struct array;
using value = std::variant<
    std::nullptr_t,
    std::unique_ptr<object>,
    std::unique_ptr<array>,
    std::string,
    double,
    int,
    bool
>;
class object : public std::unordered_map<std::string, value>
{};
class array : public std::vector<value>
{};

value read(std::string_view str);
void write(value val, std::string& str);
}

namespace ujson
{
namespace detail
{

inline void fstrip(std::string_view& str)
{
    while (!str.empty() && std::isspace(str.front()))
    {
        str.remove_prefix(1);
    }
}

inline void discard(std::string_view& str, char c)
{
    if (str.empty() || str.front() != c)
    {
        throw std::runtime_error("Expected character " + std::string(1, c) + " at -" + std::to_string(str.size()));
    }
    str.remove_prefix(1);
}
inline void discard(std::string_view& str, std::string_view cs)
{
    if (str.substr(0, cs.size()) == cs)
    {
        str.remove_prefix(cs.size());
    }
    else
    {
        throw std::runtime_error("Expected character sequence " + std::string(cs) + " at -" + std::to_string(str.size()));
    }
}

template <typename T>
T read(std::string_view& str);

template <>
inline std::string read(std::string_view& str)
{
    std::string ret;
    discard(str, '"');
    bool escape = false;
    while (!str.empty() && !escape && str.front() != '"')
    {
        if (!escape && str.front() == '\\')
        {
            escape = true;
        }
        else
        {
            escape = false;
            ret += str.front();
            str.remove_prefix(1);
        }
    }
    discard(str, '"');
    return ret;
}

template <>
inline double read(std::string_view& str)
{
    double ret;
    const auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), ret);
    if (ec != std::errc())
    {
        throw std::runtime_error("Could not parse " + std::string(str) + " as double at -" +  std::to_string(str.size()));
    }
    str.remove_prefix(ptr - str.data());
    return ret;
}

template <>
inline bool read(std::string_view& str)
{
    if (str.front() == 't')
    {
        discard(str, "true");
        return true;
    }
    else if (str.front() == 'f')
    {
        discard(str, "false");
        return false;
    }
    else
    {
        throw std::runtime_error("Unexpected character " + std::string(1, str.front()) + " at -" + std::to_string(str.size()));
    }
}
template <>
inline std::nullptr_t read(std::string_view& str)
{
    discard(str, "null");
    return nullptr;
}

value read(std::string_view& str);

template <>
inline std::unique_ptr<object> read(std::string_view& str)
{
    auto ret = std::make_unique<object>();
    discard(str, '{');
    fstrip(str);
    int i = 0;
    while (str.front() != '}')
    {
        if (i++)
        {
            discard(str, ',');
        }
        fstrip(str);
        const auto k = read<std::string>(str);
        discard(str, ':');
        fstrip(str);
        ret->emplace(k, read(str));
        fstrip(str);
    }
    discard(str, '}');
    return ret;
}

template <>
inline std::unique_ptr<array> read(std::string_view& str)
{
    auto ret = std::make_unique<array>();
    discard(str, '[');
    fstrip(str);
    int i = 0;
    while (str.front() != ']')
    {
        if (i++)
        {
            discard(str, ',');
        }
        fstrip(str);
        ret->emplace_back(read(str));
        fstrip(str);
    }
    discard(str, ']');
    return ret;
}

inline value read(std::string_view& str)
{
    fstrip(str);
    if (str.empty())
    {
        return value(nullptr);
    }
    switch(str.front())
    {
        case '{':
            return value(read<std::unique_ptr<object>>(str));
        case '[':
            return value(read<std::unique_ptr<array>>(str));
        case '"':
            return value(read<std::string>(str));
        case 't':
        case 'f':
            return value(read<bool>(str));
        case 'n':
            return value(read<std::nullptr_t>(str));
        default:
            if (str.front() == '-' || ('0' <= str.front() && str.front() <= '9'))
            {
                const auto d = read<double>(str);
                double i;
                if (std::modf(d, &i) == 0.0)
                {
                    return value(static_cast<int>(i));
                }
                else
                {
                    return value(d);
                }
            }
            else
            {
                throw std::runtime_error("Unexpected character " + std::to_string(str.front()) + " at -" + std::to_string(str.size()));
            }
    }
}

inline std::string write(const value& val, int indent = 0)
{
    std::string ret;
    struct {
        std::string& ret;
        int& indent;
        void operator()(const std::unique_ptr<object>& obj)
        {
            ret += "{\n";
            int i = 0;
            indent += 4;
            for (const auto& [k, v] : *obj)
            {
                if (i++)
                {
                    ret += ",\n";
                }
                ret += std::string(indent, ' ') + k + ": " + write(v, indent);
            }
            indent -= 4;
            ret += '\n' + std::string(indent, ' ') + '}';
        }
        void operator()(const std::unique_ptr<array>& arr)
        {
            ret += "[\n";
            int i = 0;
            indent += 4;
            for (const auto& e : *arr)
            {
                if (i++)
                {
                    ret += ",\n";
                }
                ret += std::string(indent, ' ') + write(e, indent);
            }
            indent -= 4;
            ret += '\n' + std::string(indent, ' ') + ']';
        }
        void operator()(const std::string& str)
        {
            ret += str;
        }
        void operator()(double d)
        {
            std::array<char, 32> buf = {};
            const auto [ptr, ec] = std::to_chars(
                buf.data(),
                buf.data() + buf.size(),
                d,
                std::chars_format::fixed,
                std::numeric_limits<double>::max_digits10
            );
            if (ec != std::errc())
            {
                throw std::runtime_error("Could not convert double to chars");
            }
            ret += std::string_view(buf.data(), ptr - buf.data());
        }
        void operator()(int i)
        {
            std::array<char, 32> buf = {};
            const auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), i);
            if (ec != std::errc())
            {
                throw std::runtime_error("Could not convert int to chars");
            }
            ret += std::string_view(buf.data(), ptr - buf.data());
        }
        void operator()(bool b)
        {
            ret += ((b) ? "true" : "false");
        }
        void operator()(std::nullptr_t)
        {
            ret += "null";
        }
    } v = {ret, indent};
    std::visit(v, val);
    return ret;
}
} // namespace detail

inline value read(std::string_view str)
{
    return detail::read(str);
}

inline std::string write(const value& val)
{
    return detail::write(val) + '\n';
}
} // namespace ujson
