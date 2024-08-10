/*
 * Copyright (C) 2024, Nils Moehrle
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE file for details.
 */

#pragma once

#include <array>
#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <map>
#include <variant>
#include <vector>

namespace ujson
{
struct value;
using object = std::map<std::string, value>;
using array = std::vector<value>;
using variant = std::variant<
    std::nullptr_t,
    object,
    array,
    std::string,
    double,
    int,
    bool
>;

struct value : public variant {
    using variant::operator=;
    using variant::variant;
};

value parse(std::string_view);
std::string serialize(const value&);
} // namespace ujson

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
T parse(std::string_view& str);

template <>
inline std::string parse(std::string_view& str)
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
inline double parse(std::string_view& str)
{
    double ret;
    const auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), ret);
    if (ec == std::errc())
    {
        str.remove_prefix(ptr - str.data());
        return ret;
    }
    throw std::runtime_error("Could not parse " + std::string(str) + " as double at -" +  std::to_string(str.size()));
}

template <>
inline bool parse(std::string_view& str)
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
inline std::nullptr_t parse(std::string_view& str)
{
    discard(str, "null");
    return nullptr;
}

value parse(std::string_view& str);

template <>
inline object parse(std::string_view& str)
{
    auto ret = object();
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
        const auto k = parse<std::string>(str);
        discard(str, ':');
        fstrip(str);
        ret.emplace(k, parse(str));
        fstrip(str);
    }
    discard(str, '}');
    return ret;
}

template <>
inline array parse(std::string_view& str)
{
    auto ret = array();
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
        ret.emplace_back(parse(str));
        fstrip(str);
    }
    discard(str, ']');
    return ret;
}

inline value parse(std::string_view& str)
{
    fstrip(str);
    if (str.empty())
    {
        return value(nullptr);
    }
    switch(str.front())
    {
        case '{':
            return value(parse<object>(str));
        case '[':
            return value(parse<array>(str));
        case '"':
            return value(parse<std::string>(str));
        case 't':
        case 'f':
            return value(parse<bool>(str));
        case 'n':
            return value(parse<std::nullptr_t>(str));
        default:
            if (str.front() == '-' || ('0' <= str.front() && str.front() <= '9'))
            {
                const auto d = parse<double>(str);
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

inline std::string serialize(const value& val, int indent = 0)
{
    std::string ret;
    struct {
        std::string& ret;
        int& indent;
        void operator()(const object& obj)
        {
            ret += "{\n";
            int i = 0;
            indent += 4;
            for (const auto& [k, v] : obj)
            {
                if (i++)
                {
                    ret += ",\n";
                }
                ret += std::string(indent, ' ') + '"' +  k + "\": " + serialize(v, indent);
            }
            indent -= 4;
            ret += '\n' + std::string(indent, ' ') + '}';
        }
        void operator()(const array& arr)
        {
            ret += "[\n";
            int i = 0;
            indent += 4;
            for (const auto& e : arr)
            {
                if (i++)
                {
                    ret += ",\n";
                }
                ret += std::string(indent, ' ') + serialize(e, indent);
            }
            indent -= 4;
            ret += '\n' + std::string(indent, ' ') + ']';
        }
        void operator()(const std::string& str)
        {
            ret += '"' + str + '"';
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
            if (ec == std::errc())
            {
                ret += std::string_view(buf.data(), ptr - buf.data());
            }
            else
            {
                throw std::runtime_error("Could not convert double to chars");
            }
        }
        void operator()(int i)
        {
            std::array<char, 32> buf = {};
            const auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), i);
            if (ec == std::errc())
            {
                ret += std::string_view(buf.data(), ptr - buf.data());
            }
            else
            {
                throw std::runtime_error("Could not convert int to chars");
            }
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

inline value parse(std::string_view str)
{
    return detail::parse(str);
}

inline std::string serialize(const value& val)
{
    return detail::serialize(val) + '\n';
}
} // namespace ujson
