# Mirco json library

Allows json parsing and serialization of standard library types.


## API
The entire api is:
```
namespace ujson
{
struct value;
using object = std::map<std::string, value>;
using array = std::vector<value>;
struct value : public std::variant<
    std::nullptr_t,
    object,
    array,
    std::string,
    double,
    int,
    bool
>
{};

value parse(std::string_view);
std::string serialize(const value&);
}
```

## Limitations
- Requires a standard library implementation that allows incomplete types as
  values for `std::map` which is not required by the standard.
- No support for 64bit integers
- Requires the entire json to fit into memory
