#pragma once

#include <cstddef>
#include <iterator>
#include <string>
#include <utility>
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
  long long value;

public:
  Number(long long value);
  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] const long long &get() const;
  long long &get();
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

class Literal {
  std::variant<Nil, Boolean, Number, Float, String> inner;

public:
  Literal() = default;
  Literal(Nil nil);
  Literal(Boolean boolean);
  Literal(Number num);
  Literal(Float num);
  Literal(String &&string);

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
