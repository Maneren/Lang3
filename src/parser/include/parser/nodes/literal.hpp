#pragma once

#include "detail.hpp"
#include <format>
#include <string>
#include <variant>

namespace l3::ast {

struct Nil {
  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::indent(out, depth);
    std::format_to(out, "Nil\n");
  }
};

class Boolean {
  bool value;

public:
  Boolean(bool value) : value(value) {}
  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::indent(out, depth);
    std::format_to(out, "Boolean {}\n", value);
  }
};

class Num {
  long long value;

public:
  Num(long long value) : value(value) {}
  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::indent(out, depth);
    std::format_to(out, "Num {}\n", value);
  }
};

class String {
  std::string value;

public:
  String(std::string &&value) : value(std::move(value)) {}
  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::indent(out, depth);
    std::format_to(out, "String {}\n", value);
  }
};
class Literal {
  std::variant<Nil, Boolean, Num, String> inner;

public:
  Literal() = default;
  Literal(Nil nil) : inner(nil) {}
  Literal(Boolean boolean) : inner(boolean) {}
  Literal(Num num) : inner(num) {}
  Literal(String &&string) : inner(std::move(string)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::indent(out, depth);
    std::format_to(out, "Literal\n");
    inner.visit([&out, depth](const auto &node) -> void {
      node.print(out, depth + 1);
    });
  }
};

} // namespace l3::ast
