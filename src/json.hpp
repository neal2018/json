#ifndef JSON_H
#define JSON_H 1

#include <expected>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace json {

struct Node;

using Null = std::monostate;
using Boolean = bool;
using Integer = int64_t;
using Float = double;
using String = std::string;
using Array = std::vector<Node>;
using Object = std::map<std::string, Node>;
using Value = std::variant<Null, Boolean, Integer, Float, String, Array, Object>;

struct Node {
  Value value;
  Node(Value _value) : value(_value) {}
  Node() : value(Null{}) {}
  auto operator<=>(const Node&) const = default;
  auto operator[](const std::string& key) const {
    if (auto* object = std::get_if<Object>(&value)) {
      return object->at(key);
    }
    throw std::runtime_error("not an object");
  }
  auto operator[](size_t index) const {
    if (auto* array = std::get_if<Array>(&value)) {
      return array->at(index);
    }
    throw std::runtime_error("not an array");
  }
};

enum class JsonError {
  parse_expect_value,
  parse_invalid_value,
  parse_root_not_singular,
  parse_number_too_big,
};

class JsonParser {
public:
  std::string_view json_str;
  size_t pos = 0;

  auto parse_whitespace() -> void {
    while (pos < json_str.size() && std::isspace(json_str[pos])) {
      ++pos;
    }
  }

  auto parse_null() -> std::expected<Value, JsonError> {
    parse_whitespace();
    if (json_str.substr(pos, 4) == "null") {
      pos += 4;
      return Null{};
    }
    return std::unexpected{JsonError::parse_invalid_value};
  }

  auto parse_true() -> std::expected<Value, JsonError> {
    parse_whitespace();
    if (json_str.substr(pos, 4) == "true") {
      pos += 4;
      return true;
    }
    return std::unexpected{JsonError::parse_invalid_value};
  }

  auto parse_false() -> std::expected<Value, JsonError> {
    parse_whitespace();
    if (json_str.substr(pos, 5) == "false") {
      pos += 5;
      return false;
    }
    return std::unexpected{JsonError::parse_invalid_value};
  }

  auto parse_number() -> std::expected<Value, JsonError> {
    parse_whitespace();
    if (pos == json_str.size()) {
      return std::unexpected{JsonError::parse_invalid_value};
    }
    size_t end_pos = pos;
    if (json_str[end_pos] == '-') {
      ++end_pos;
    }
    if (end_pos == json_str.size()) {
      return std::unexpected{JsonError::parse_invalid_value};
    }
    if (json_str[end_pos] == '0') {
      ++end_pos;
    } else {
      if (!std::isdigit(json_str[end_pos])) {
        return std::unexpected{JsonError::parse_invalid_value};
      }
      while (end_pos < json_str.size() && std::isdigit(json_str[end_pos])) {
        ++end_pos;
      }
    }

    // handle interger
    if (end_pos == json_str.size() ||
        (json_str[end_pos] != '.' && json_str[end_pos] != 'e' && json_str[end_pos] != 'E')) {
      try {
        auto number = std::stoll(std::string{json_str.substr(pos, end_pos - pos)});
        pos = end_pos;
        return number;
      } catch (const std::out_of_range&) {
        return std::unexpected{JsonError::parse_number_too_big};
      } catch (const std::invalid_argument&) {
        return std::unexpected{JsonError::parse_invalid_value};
      }
    }

    if (end_pos < json_str.size() && json_str[end_pos] == '.') {
      ++end_pos;
      if (end_pos == json_str.size() || !std::isdigit(json_str[end_pos])) {
        return std::unexpected{JsonError::parse_invalid_value};
      }
      while (end_pos < json_str.size() && std::isdigit(json_str[end_pos])) {
        ++end_pos;
      }
    }
    if (end_pos < json_str.size() && (json_str[end_pos] == 'e' || json_str[end_pos] == 'E')) {
      ++end_pos;
      if (end_pos == json_str.size() || (json_str[end_pos] != '+' && json_str[end_pos] != '-' &&
                                         !std::isdigit(json_str[end_pos]))) {
        return std::unexpected{JsonError::parse_invalid_value};
      }
      ++end_pos;
      while (end_pos < json_str.size() && std::isdigit(json_str[end_pos])) {
        ++end_pos;
      }
    }
    try {
      const auto number = std::stod(std::string{json_str.substr(pos, end_pos - pos)});
      pos = end_pos;
      return number;
    } catch (const std::out_of_range&) {
      return std::unexpected{JsonError::parse_number_too_big};
    } catch (const std::invalid_argument&) {
      return std::unexpected{JsonError::parse_invalid_value};
    }
  }

  auto parse_string() -> std::expected<Value, JsonError> {
    parse_whitespace();
    if (pos == json_str.size() || json_str[pos] != '"') {
      return std::unexpected{JsonError::parse_invalid_value};
    }
    ++pos;
    std::string str;
    static const auto escape_mappping = std::unordered_map<char, char>{
        {'"', '"'},  {'\\', '\\'}, {'/', '/'},  {'b', '\b'},
        {'f', '\f'}, {'n', '\n'},  {'r', '\r'}, {'t', '\t'},
    };
    while (pos < json_str.size() && json_str[pos] != '"') {
      if (json_str[pos] == '\\') {
        ++pos;
        if (pos == json_str.size()) {
          return std::unexpected{JsonError::parse_invalid_value};
        }
        if (const auto it = escape_mappping.find(json_str[pos]); it != escape_mappping.end()) {
          str += it->second;
        } else if (json_str[pos] == 'u') {
          str += parse_unicode();
        } else {
          return std::unexpected{JsonError::parse_invalid_value};
        }
      } else {
        str += json_str[pos];
      }
      ++pos;
    }
    if (pos == json_str.size() || json_str[pos] != '"') {
      return std::unexpected{JsonError::parse_invalid_value};
    }
    ++pos;
    return str;
  }

  auto parse_unicode() -> char {
    parse_whitespace();
    if (pos + 4 >= json_str.size()) {
      return '\0';
    }
    const auto hex_str = std::string{json_str.substr(pos + 1, 4)};
    pos += 4;
    return static_cast<char>(std::stoi(hex_str, nullptr, 16));
  }

  auto parse_array() -> std::expected<Value, JsonError> {
    parse_whitespace();
    if (pos == json_str.size() || json_str[pos] != '[') {
      return std::unexpected{JsonError::parse_invalid_value};
    }
    ++pos;
    Array array;
    while (pos < json_str.size() && json_str[pos] != ']') {
      const auto value = parse_value();
      if (!value) {
        return std::unexpected{value.error()};
      }
      array.push_back(Node{value.value()});
      parse_whitespace();
      if (pos < json_str.size() && json_str[pos] == ',') {
        ++pos;
      }
      parse_whitespace();
    }
    if (pos == json_str.size() || json_str[pos] != ']') {
      return std::unexpected{JsonError::parse_invalid_value};
    }
    ++pos;
    return array;
  }

  auto parse_object() -> std::expected<Value, JsonError> {
    parse_whitespace();
    if (pos == json_str.size() || json_str[pos] != '{') {
      return std::unexpected{JsonError::parse_invalid_value};
    }
    ++pos;
    Object object;
    while (pos < json_str.size() && json_str[pos] != '}') {
      const auto key = parse_string();
      if (!key) {
        return std::unexpected{key.error()};
      }
      if (!std::holds_alternative<std::string>(key.value())) {
        return std::unexpected{JsonError::parse_invalid_value};
      }
      parse_whitespace();
      if (pos == json_str.size() || json_str[pos] != ':') {
        return std::unexpected{JsonError::parse_invalid_value};
      }
      ++pos;
      const auto value = parse_value();
      if (!value) {
        return std::unexpected{value.error()};
      }
      object.emplace(std::get<std::string>(key.value()), Node{value.value()});
      parse_whitespace();
      if (pos < json_str.size() && json_str[pos] == ',') {
        ++pos;
      }
      parse_whitespace();
    }
    if (pos == json_str.size() || json_str[pos] != '}') {
      return std::unexpected{JsonError::parse_invalid_value};
    }
    ++pos;
    return object;
  }

  auto parse_value() -> std::expected<Value, JsonError> {
    parse_whitespace();
    if (pos == json_str.size()) {
      return std::unexpected{JsonError::parse_expect_value};
    }
    switch (json_str[pos]) {
      case 'n':
        return parse_null();
      case 't':
        return parse_true();
      case 'f':
        return parse_false();
      case '"':
        return parse_string();
      case '[':
        return parse_array();
      case '{':
        return parse_object();
      default:
        return parse_number();
    }
  }

  auto parse() -> std::expected<Node, JsonError> {
    parse_whitespace();
    auto value = parse_value();
    if (!value) {
      return std::unexpected{value.error()};
    }
    parse_whitespace();
    if (pos != json_str.size()) {
      return std::unexpected{JsonError::parse_root_not_singular};
    }
    return Node{*value};
  }
};

inline auto parse(std::string_view json_str) -> std::expected<Node, JsonError> {
  JsonParser parser{json_str};
  return parser.parse();
}

class JsonGenerator {
public:
  static auto generate(const Node& node) -> std::string {
    return std::visit(
        [](const auto& value) -> std::string {
          if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Null>) {
            return "null";
          } else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Boolean>) {
            return value ? "true" : "false";
          } else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Integer>) {
            return std::to_string(value);
          } else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Float>) {
            return std::to_string(value);
          } else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, String>) {
            return generate_string(value);
          } else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Array>) {
            return generate_array(value);
          } else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Object>) {
            return generate_object(value);
          }
        },
        node.value);
  }
  static auto generate_string(const String& str) -> std::string {
    std::string json_str;
    json_str += '"';
    static const auto escape_map = std::unordered_map<char, std::string>{
        {'"', "\\\""}, {'\\', "\\\\"}, {'/', "\\/"},  {'\b', "\\b"},
        {'\f', "\\f"}, {'\n', "\\n"},  {'\r', "\\r"}, {'\t', "\\t"}};
    for (const auto& c : str) {
      if (const auto it = escape_map.find(c); it != escape_map.end()) {
        json_str += it->second;
      } else if (c < 0x20) {
        json_str += "\\u";
        json_str += std::string(4 - std::to_string(static_cast<int>(c)).size(), '0');
        json_str += std::to_string(static_cast<int>(c));
      } else {
        json_str += c;
      }
    }
    json_str += '"';
    return json_str;
  }
  static auto generate_array(const Array& array) -> std::string {
    std::string json_str;
    json_str += '[';
    for (const auto& node : array) {
      json_str += generate(node);
      json_str += ',';
    }
    if (!array.empty()) {
      json_str.pop_back();
    }
    json_str += ']';
    return json_str;
  }
  static auto generate_object(const Object& object) -> std::string {
    std::string json_str;
    json_str += '{';
    for (const auto& [key, node] : object) {
      json_str += generate_string(key);
      json_str += ':';
      json_str += generate(node);
      json_str += ',';
    }
    if (!object.empty()) {
      json_str.pop_back();
    }
    json_str += '}';
    return json_str;
  }
};

inline auto generate(const Node& node) -> std::string {
  JsonGenerator generator;
  return generator.generate(node);
}

inline auto operator<<(auto& out, const Node& node) -> auto& {
  out << JsonGenerator::generate(node);
  return out;
}

}  // namespace json

#endif  // JSON_H
