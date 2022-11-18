# json

A lightweight json parser and generator in modern C++. It is header-only and has only 300 lines of codes. It requires at least g++ 12.1 and C++23.

### Example

#### Parser

```cpp
#include <iostream>
#include <string>
#include "json.hpp"

int main() {
  std::string_view json_str = R"(
        {
            "name": "json",
            "author": "neal",
            "version": "0.1",
            "pi": 3.1415926,
            "features": [
                "parser",
                "generator"
            ]
        }
    )";

  auto obj = json::parse(json_str).value();
  std::cout << obj["name"] << std::endl;
  std::cout << obj["author"] << std::endl;
  std::cout << obj["version"] << std::endl;
  std::cout << obj["features"][0] << std::endl;
  std::cout << obj["features"][1] << std::endl;
}
```
#### Generator

```cpp
#include <iostream>
#include "json.hpp"

int main() {
  auto obj = json::Node{
      json::Object{
          {"name", json::Node{"json"}},
          {"author", json::Node{"neal"}},
          {"version", json::Node{"0.1"}},
          {"pi", json::Node{3.1415926}},
          {"features", json::Node{json::Array{
                           json::Node{"parser"},
                           json::Node{"generator"},
                       }}},
      },
  };
  std::cout << json::generate(obj) << std::endl;
  // output: 
  // {"author":"neal","features":["parser","generator"],
  //  "name":"json","pi":3.141593,"version":"0.1"}
}
```
