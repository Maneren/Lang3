#pragma once

#include "ast/nodes/expression_list.hpp"
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

  [[nodiscard]] const bool &get() const;
  bool &get();
};

class Number {
  std::int64_t value;

public:
  Number(std::int64_t value);
  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] const std::int64_t &get() const;
  std::int64_t &get();
};

class Float {
  double value;

public:
  Float(double value);
  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] const double &get() const;
  double &get();
};

class String {
  std::string value;

public:
  String(const std::string &literal);
  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] const std::string &get() const;
  std::string &get();
};

class Array {
  ExpressionList elements;

public:
  Array();
  Array(ExpressionList &&elements);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] const ExpressionList &get() const;
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

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    inner.visit([&out, depth](const auto &node) -> void {
      node.print(out, depth);
    });
  }

  [[nodiscard]] const auto &get() const { return inner; }
  auto &get() { return inner; }
};

} // namespace l3::ast
