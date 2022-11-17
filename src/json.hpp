#ifndef JSON_H
#define JSON_H 1

#include <expected>
#include <map>
#include <stdexcept>
#include <string>
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
  Node(Value value) : value(value) {}
  Node() : value(Null{}) {}
  auto operator<=>(const Node&) const = default;
};

enum class JsonError {
  parse_expect_value,
  parse_invalid_value,
  parse_root_not_singular,
  parse_number_too_big,
};

class JsonParser {
public:
  std::string_view json_str_;
  size_t pos_ = 0;

  auto parse_whitespace() -> void {
    while (pos_ < json_str_.size() && std::isspace(json_str_[pos_])) {
      ++pos_;
    }
  }

  auto parse_null() -> std::expected<Value, JsonError> {
    if (json_str_.substr(pos_, 4) == "null") {
      pos_ += 4;
      return Null{};
    }
    return std::unexpected{JsonError::parse_invalid_value};
  }

  auto parse_true() -> std::expected<Value, JsonError> {
    if (json_str_.substr(pos_, 4) == "true") {
      pos_ += 4;
      return true;
    }
    return std::unexpected{JsonError::parse_invalid_value};
  }

  auto parse_false() -> std::expected<Value, JsonError> {
    if (json_str_.substr(pos_, 5) == "false") {
      pos_ += 5;
      return false;
    }
    return std::unexpected{JsonError::parse_invalid_value};
  }

  auto parse_number() -> std::expected<Value, JsonError> {
    if (pos_ == json_str_.size()) {
      return std::unexpected{JsonError::parse_invalid_value};
    }
    size_t end_pos = pos_;
    if (json_str_[end_pos] == '-') {
      ++end_pos;
    }
    if (end_pos == json_str_.size()) {
      return std::unexpected{JsonError::parse_invalid_value};
    }
    if (json_str_[end_pos] == '0') {
      ++end_pos;
    } else {
      if (!std::isdigit(json_str_[end_pos])) {
        return std::unexpected{JsonError::parse_invalid_value};
      }
      while (end_pos < json_str_.size() && std::isdigit(json_str_[end_pos])) {
        ++end_pos;
      }
    }

    // handle interger
    if (end_pos == json_str_.size() || json_str_[end_pos] != '.' || json_str_[end_pos] != 'e' ||
        json_str_[end_pos] != 'E') {
      try {
        auto number = std::stoll(std::string{json_str_.substr(pos_, end_pos - pos_)});
        pos_ = end_pos;
        return number;
      } catch (const std::out_of_range&) {
        return std::unexpected{JsonError::parse_number_too_big};
      } catch (const std::invalid_argument&) {
        return std::unexpected{JsonError::parse_invalid_value};
      }
    }

    if (end_pos < json_str_.size() && json_str_[end_pos] == '.') {
      ++end_pos;
      if (end_pos == json_str_.size() || !std::isdigit(json_str_[end_pos])) {
        return std::unexpected{JsonError::parse_invalid_value};
      }
      while (end_pos < json_str_.size() && std::isdigit(json_str_[end_pos])) {
        ++end_pos;
      }
    }
    if (end_pos < json_str_.size() && (json_str_[end_pos] == 'e' || json_str_[end_pos] == 'E')) {
      ++end_pos;
      if (end_pos == json_str_.size() || (json_str_[end_pos] != '+' && json_str_[end_pos] != '-' &&
                                          !std::isdigit(json_str_[end_pos]))) {
        return std::unexpected{JsonError::parse_invalid_value};
      }
      ++end_pos;
      while (end_pos < json_str_.size() && std::isdigit(json_str_[end_pos])) {
        ++end_pos;
      }
    }
    try {
      auto number = std::stod(std::string{json_str_.substr(pos_, end_pos - pos_)});
      pos_ = end_pos;
      return number;
    } catch (const std::out_of_range&) {
      return std::unexpected{JsonError::parse_number_too_big};
    } catch (const std::invalid_argument&) {
      return std::unexpected{JsonError::parse_invalid_value};
    }
  }

  auto parse_string() -> std::expected<Value, JsonError> {
    if (pos_ == json_str_.size() || json_str_[pos_] != '"') {
      return std::unexpected{JsonError::parse_invalid_value};
    }
    ++pos_;
    std::string str;
    static auto escape_mappping = std::map<char, char>{
        {'"', '"'},  {'\\', '\\'}, {'/', '/'},  {'b', '\b'},
        {'f', '\f'}, {'n', '\n'},  {'r', '\r'}, {'t', '\t'},
    };
    while (pos_ < json_str_.size() && json_str_[pos_] != '"') {
      if (json_str_[pos_] == '\\') {
        ++pos_;
        if (pos_ == json_str_.size()) {
          return std::unexpected{JsonError::parse_invalid_value};
        }
        if (auto it = escape_mappping.find(json_str_[pos_]); it != escape_mappping.end()) {
          str += it->second;
        } else if (json_str_[pos_] == 'u') {
          str += parse_unicode();
        } else {
          return std::unexpected{JsonError::parse_invalid_value};
        }
      } else {
        str += json_str_[pos_];
      }
      ++pos_;
    }
    if (pos_ == json_str_.size() || json_str_[pos_] != '"') {
      return std::unexpected{JsonError::parse_invalid_value};
    }
    ++pos_;
    return str;
  }

  auto parse_unicode() -> char {
    if (pos_ + 4 >= json_str_.size()) {
      return '\0';
    }
    auto hex_str = std::string{json_str_.substr(pos_ + 1, 4)};
    pos_ += 4;
    return static_cast<char>(std::stoi(hex_str, nullptr, 16));
  }

  auto parse_array() -> std::expected<Value, JsonError> {
    if (pos_ == json_str_.size() || json_str_[pos_] != '[') {
      return std::unexpected{JsonError::parse_invalid_value};
    }
    ++pos_;
    Array array;
    while (pos_ < json_str_.size() && json_str_[pos_] != ']') {
      auto value = parse_value();
      if (!value) {
        return std::unexpected{value.error()};
      }
      array.push_back(Node{value.value()});
      parse_whitespace();
      if (pos_ < json_str_.size() && json_str_[pos_] == ',') {
        ++pos_;
      }
    }
    if (pos_ == json_str_.size() || json_str_[pos_] != ']') {
      return std::unexpected{JsonError::parse_invalid_value};
    }
    ++pos_;
    return array;
  }

  auto parse_object() -> std::expected<Value, JsonError> {
    if (pos_ == json_str_.size() || json_str_[pos_] != '{') {
      return std::unexpected{JsonError::parse_invalid_value};
    }
    ++pos_;
    Object object;
    while (pos_ < json_str_.size() && json_str_[pos_] != '}') {
      const auto key = parse_string();
      if (!key) {
        return std::unexpected{key.error()};
      }
      if (!std::holds_alternative<std::string>(key.value())) {
        return std::unexpected{JsonError::parse_invalid_value};
      }
      parse_whitespace();
      if (pos_ == json_str_.size() || json_str_[pos_] != ':') {
        return std::unexpected{JsonError::parse_invalid_value};
      }
      ++pos_;
      auto value = parse_value();
      if (!value) {
        return std::unexpected{value.error()};
      }
      object.emplace(std::get<std::string>(key.value()), Node{value.value()});
      parse_whitespace();
      if (pos_ < json_str_.size() && json_str_[pos_] == ',') {
        ++pos_;
      }
    }
    if (pos_ == json_str_.size() || json_str_[pos_] != '}') {
      return std::unexpected{JsonError::parse_invalid_value};
    }
    ++pos_;
    return object;
  }

  auto parse_value() -> std::expected<Value, JsonError> {
    if (pos_ == json_str_.size()) {
      return std::unexpected{JsonError::parse_expect_value};
    }
    switch (json_str_[pos_]) {
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
    if (pos_ != json_str_.size()) {
      return std::unexpected{JsonError::parse_root_not_singular};
    }
    return Node{*value};
  }
};

inline auto json_parse(std::string_view json_str) -> std::expected<Node, JsonError> {
  JsonParser parser{json_str};
  return parser.parse();
}

class JsonGenerator {
public:
  auto generate(const Node& node) -> std::string {
    return std::visit(
        [this](const auto& value) -> std::string {
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
  auto generate_string(const String& str) -> std::string {
    std::string json_str;
    json_str += '"';
    static auto escape_map =
        std::map<char, std::string>{{'"', "\\\""}, {'\\', "\\\\"}, {'/', "\\/"},  {'\b', "\\b"},
                                    {'\f', "\\f"}, {'\n', "\\n"},  {'\r', "\\r"}, {'\t', "\\t"}};
    for (const auto& c : str) {
      if (escape_map.find(c) != escape_map.end()) {
        json_str += escape_map[c];
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
  auto generate_array(const Array& array) -> std::string {
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
  auto generate_object(const Object& object) -> std::string {
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

inline auto json_generate(const Node& node) -> std::string {
  JsonGenerator generator;
  return generator.generate(node);
}

}  // namespace json

#endif  // JSON_H
