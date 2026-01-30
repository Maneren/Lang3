module;

#include <cstdint>
#include <iterator>
#include <string>
#include <utils/accessor.h>

export module l3.ast:literal_parts;

import :printing;

export namespace l3::ast {

struct Nil {
  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Nil");
  }
};

class Boolean {
  bool value;

public:
  Boolean(bool value) : value(value) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Boolean {}", value);
  }

  DEFINE_ACCESSOR_X(value);
};

class Number {
  std::int64_t value;

public:
  Number(std::int64_t value) : value(value) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Number {}", value);
  }

  DEFINE_ACCESSOR_X(value);
};

class Float {
  double value;

public:
  Float(double value) : value(value) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Float {}", value);
  }

  DEFINE_ACCESSOR_X(value);
};

class String {
  std::string value;

public:
  String(const std::string &literal);

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "String \"{}\"", value);
  }

  DEFINE_ACCESSOR_X(value);
};

} // namespace l3::ast
