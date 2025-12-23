#pragma once

#include <string>
#include <variant>

namespace l3::ast {

struct Nil {
  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

class Boolean {
  bool value;

public:
  Boolean(bool value) : value(value) {}
  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

class Number {
  long long value;

public:
  Number(long long value) : value(value) {}
  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

class Float {
  double value;

public:
  Float(double value) : value(value) {}
  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

class String {
  std::string value;

public:
  String(const std::string &literal);
  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

class Literal {
  std::variant<Nil, Boolean, Number, Float, String> inner;

public:
  Literal() = default;
  Literal(Nil nil) : inner(nil) {}
  Literal(Boolean boolean) : inner(boolean) {}
  Literal(Number num) : inner(num) {}
  Literal(Float num) : inner(num) {}
  Literal(String &&string) : inner(std::move(string)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    inner.visit([&out, depth](const auto &node) -> void {
      node.print(out, depth);
    });
  }
};

} // namespace l3::ast
