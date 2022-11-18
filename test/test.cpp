#include <cassert>

#include "../src/json.hpp"

void test_parse_null() {
  auto result = json::parse("null");
  assert(result);
  assert(std::holds_alternative<json::Null>((*result).value));
}

void test_parse_true() {
  auto result = json::parse("true");
  assert(result);
  assert(std::holds_alternative<json::Boolean>((*result).value));
  assert(std::get<json::Boolean>((*result).value));
}

void test_parse_false() {
  auto result = json::parse("false");
  assert(result);
  assert(std::holds_alternative<json::Boolean>((*result).value));
  assert(!std::get<json::Boolean>((*result).value));
}

void test_parse_whitespace() {
  auto result = json::parse("  null");
  assert(result);
  assert(std::holds_alternative<json::Null>((*result).value));
}

void test_parse_root_not_singular() {
  auto result = json::parse("null x");
  assert(!result);
  assert(result.error() == json::JsonError::parse_root_not_singular);
}

void test_parse_invalid_value() {
  auto result = json::parse("nul");
  assert(!result);
  assert(result.error() == json::JsonError::parse_invalid_value);
}

void test_parse_expect_value() {
  auto result = json::parse(std::string(""));
  assert(!result);
  assert(result.error() == json::JsonError::parse_expect_value);
}

void test_parse_float_too_big() {
  auto result = json::parse("1e30009");
  assert(!result);
  assert(result.error() == json::JsonError::parse_number_too_big);
}

void test_parse_float_helper(std::string json_str, json::Boolean expected) {
  constexpr json::Boolean eps = 1e-12;
  auto result = json::parse(json_str);
  assert(result);
  assert(std::holds_alternative<json::Float>((*result).value));
  assert(std::abs(std::get<json::Float>((*result).value) - expected) < eps);
}

void test_parse_float() {
  test_parse_float_helper("0", 0.0);
  test_parse_float_helper("0.0", 0.0);
  test_parse_float_helper("0.5", 0.5);
  test_parse_float_helper("-0.5", -0.5);
  test_parse_float_helper("1", 1.0);
  test_parse_float_helper("-1", -1.0);
  test_parse_float_helper("1.5", 1.5);
  test_parse_float_helper("-1.5", -1.5);
  test_parse_float_helper("3.1416", 3.1416);
  test_parse_float_helper("1E10", 1E10);
  test_parse_float_helper("1e10", 1e10);
  test_parse_float_helper("1E+10", 1E+10);
  test_parse_float_helper("1E-10", 1E-10);
  test_parse_float_helper("-1E10", -1E10);
  test_parse_float_helper("-1e10", -1e10);
  test_parse_float_helper("-1E+10", -1E+10);
  test_parse_float_helper("-1E-10", -1E-10);
  test_parse_float_helper("1.234E+10", 1.234E+10);
  test_parse_float_helper("1.234E-10", 1.234E-10);
}

void test_parse_integer_too_big() {
  auto result = json::parse("100000000000000000000000000000000000000000000000");
  assert(!result);
  assert(result.error() == json::JsonError::parse_number_too_big);
}

void test_parse_integer_helper(std::string json_str, json::Integer expected) {
  auto result = json::parse(json_str);
  assert(result);
  assert(std::holds_alternative<json::Integer>((*result).value));
  assert(std::get<json::Integer>((*result).value) == expected);
}

void test_parse_integer() {
  test_parse_integer_helper("0", 0);
  test_parse_integer_helper("1", 1);
  test_parse_integer_helper("-1", -1);
  test_parse_integer_helper("123", 123);
  test_parse_integer_helper("-123", -123);
}

void test_parse_string_invalid_escape() {
  auto result = json::parse(R"("abc\k")");
  assert(!result);
  assert(result.error() == json::JsonError::parse_invalid_value);
}

void test_parse_string_invalid_unicode_hex() {
  auto result = json::parse(R"("abc\u123k")");
  assert(!result);
  assert(result.error() == json::JsonError::parse_invalid_value);
}

void test_parse_string_invalid_unicode_surrogate() {
  auto result = json::parse(R"("abc\u1234\u5678")");
  assert(!result);
  assert(result.error() == json::JsonError::parse_invalid_value);
}

void test_parse_string_missing_quotation_mark() {
  auto result = json::parse(R"("abc)");
  assert(!result);
  assert(result.error() == json::JsonError::parse_invalid_value);
}

void test_parse_string_invalid_utf8() {
  auto result = json::parse(R"("abc\ud800")");
  assert(!result);
  assert(result.error() == json::JsonError::parse_invalid_value);
}

void test_parse_array() {
  auto result =
      json::parse(R"([null, true, false, 123, "abc", [1, 2, 3], {"a": 1, "b": 2, "c": 3}])");
  assert(result);
  assert(std::holds_alternative<json::Array>((*result).value));
  assert(std::get<json::Array>((*result).value).size() == 7);
}

void test_parse_object() {
  auto result = json::parse(R"({"a": 1, "b": 2, "c": 3})");
  assert(result);
  assert(std::holds_alternative<json::Object>((*result).value));
  assert(std::get<json::Object>((*result).value).size() == 3);
}

void test_parse_object_miss_key() {
  auto result = json::parse(R"({"a": 1, "b": 2, : 3})");
  assert(!result);
  assert(result.error() == json::JsonError::parse_invalid_value);
}

void test_parse_object_miss_colon() {
  auto result = json::parse(R"({"a": 1, "b": 2, "c" 3})");
  assert(!result);
  assert(result.error() == json::JsonError::parse_invalid_value);
}

void test_parse_object_miss_comma_or_curly_bracket() {
  auto result = json::parse(R"({"a": 1, "b": 2, "c" 3)");
  assert(!result);
  assert(result.error() == json::JsonError::parse_invalid_value);
}

void test_parse_complex() {
  auto result = json::parse(R"(
    {
      "n": null,
      "t": true,
      "f": false,
      "i": 123,
      "s": "abc",
      "a": [1, 2, 3],
      "o": {
        "1": 1,
        "2": 2,
        "3": 3
      }
    }
  )");
  auto expected = json::Object{
      {"n", json::Node{json::Null{}}},
      {"t", json::Node{true}},
      {"f", json::Node{false}},
      {"i", json::Node{123}},
      {"s", json::Node{"abc"}},
      {"a", json::Node{json::Array{json::Node{1}, json::Node{2}, json::Node{3}}}},
      {"o", json::Node{json::Object{
                {"1", json::Node{1}},
                {"2", json::Node{2}},
                {"3", json::Node{3}},
            }}},
  };
  assert(result);
  assert(std::holds_alternative<json::Object>((*result).value));
  assert(std::get<json::Object>((*result).value) == expected);
}

void test_parse_complex_miss_comma_or_square_bracket() {
  auto result = json::parse(R"(
    {
      "n": null,
      "t": true,
      "f": false,
      "i": 123,
      "s": "abc",
      "a": [1, 2, 3
      "o": {
        "1": 1,
        "2": 2,
        "3": 3
      }
    }
  )");
  assert(!result);
  assert(result.error() == json::JsonError::parse_invalid_value);
}

void test_parse() {
  test_parse_null();
  test_parse_true();
  test_parse_false();
  test_parse_whitespace();
  test_parse_root_not_singular();
  test_parse_invalid_value();
  test_parse_expect_value();
}

void test_generate_null() {
  auto result = json::generate(json::Node{json::Null{}});
  assert(result == "null");
}

void test_generate_true() {
  auto result = json::generate(json::Node{true});
  assert(result == "true");
}

void test_generate_false() {
  auto result = json::generate(json::Node{false});
  assert(result == "false");
}

void test_generate_integer() {
  auto result = json::generate(json::Node{123});
  assert(result == "123");
}

void test_generate_string() {
  auto result = json::generate(json::Node{"abc"});
  assert(result == R"("abc")");
}

void test_generate_array() {
  auto result = json::generate(json::Node{json::Array{
      json::Node{json::Null{}},
      json::Node{true},
      json::Node{false},
      json::Node{123},
      json::Node{"abc"},
      json::Node{json::Array{json::Node{1}, json::Node{2}, json::Node{3}}},
      json::Node{json::Object{
          {"a", json::Node{1}},
          {"b", json::Node{2}},
          {"c", json::Node{3}},
      }},
  }});
  assert(result == R"([null,true,false,123,"abc",[1,2,3],{"a":1,"b":2,"c":3}])");
}

void test_generate_object() {
  auto result = json::generate(json::Node{json::Object{
      {"n", json::Node{json::Null{}}},
      {"t", json::Node{true}},
      {"f", json::Node{false}},
      {"i", json::Node{123}},
      {"s", json::Node{"abc"}},
      {"a", json::Node{json::Array{json::Node{1}, json::Node{2}, json::Node{3}}}},
      {"o", json::Node{json::Object{
                {"1", json::Node{1}},
                {"2", json::Node{2}},
                {"3", json::Node{3}},
            }}},
  }});
  assert(result ==
         R"({"a":[1,2,3],"f":false,"i":123,"n":null,"o":{"1":1,"2":2,"3":3},"s":"abc","t":true})");
}

void test_generate() {
  test_generate_null();
  test_generate_true();
  test_generate_false();
  test_generate_integer();
  test_generate_string();
  test_generate_array();
  test_generate_object();
}

int main() {
  test_parse();
  test_generate();
}
