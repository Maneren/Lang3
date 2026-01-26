#pragma once

#include "ast/nodes/expression_list.hpp"
#include "utils/accessor.h"
#include "utils/match.h"
#include <cstddef>
#include <iterator>
#include <string>
#include <variant>

namespace l3::ast {

struct Nil {
  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;
};

class Boolean {
  bool value;

public:
  Boolean(bool value);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_ACCESSOR(value, bool, value)
};

class Number {
  std::int64_t value;

public:
  Number(std::int64_t value);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_ACCESSOR(value, std::int64_t, value)
};

class Float {
  double value;

public:
  Float(double value);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_ACCESSOR(value, double, value)
};

class String {
  std::string value;

public:
  String(const std::string &literal);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_ACCESSOR(value, std::string, value)
};

class Array {
  ExpressionList elements;

public:
  Array();
  Array(ExpressionList &&elements);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_ACCESSOR(elements, ExpressionList, elements)
};

class Literal {
  std::variant<Nil, Boolean, Number, Float, String, Array> inner;

public:
  Literal() = default;
  Literal(Nil nil);
  Literal(Boolean boolean);
  Literal(Number num);
  Literal(Float num);
  Literal(String &&string);
  Literal(Array &&array);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  auto visit(auto &&...visitor) const -> decltype(auto) {
    return match::match(inner, std::forward<decltype(visitor)>(visitor)...);
  }
};

} // namespace l3::ast
