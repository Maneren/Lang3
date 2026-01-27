module;

#include <deque>
#include <ranges>
#include <utils/accessor.h>
#include <utils/match.h>
#include <variant>

export module l3.ast:literal;

import :printing;

char decode_escape(char c);

export namespace l3::ast {

class Expression;

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

  DEFINE_ACCESSOR(value, bool, value)
};

class Number {
  std::int64_t value;

public:
  Number(std::int64_t value) : value(value) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Number {}", value);
  }

  DEFINE_ACCESSOR(value, std::int64_t, value)
};

class Float {
  double value;

public:
  Float(double value) : value(value) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Float {}", value);
  }

  DEFINE_ACCESSOR(value, double, value)
};

class String {
  std::string value;

public:
  String(const std::string &literal) {
    using namespace std::ranges;

    value.reserve(literal.size());

    for (auto &&[index, segment] :
         literal | views::split('\\') | views::enumerate) {
      if (index == 0) {
        // First segment has no preceding escape character
        value.append_range(segment);
      } else if (!segment.empty()) {
        value += decode_escape(segment.front());
        value.append_range(segment | views::drop(1));
      } else {
        // Do nothing if the segment is empty
      }
    }
  }

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "String \"{}\"", value);
  }

  DEFINE_ACCESSOR(value, std::string, value)
};

class ExpressionList : public std::deque<Expression> {
public:
  ExpressionList();
  ExpressionList(const ExpressionList &) = delete;
  ExpressionList(ExpressionList &&) noexcept;
  ExpressionList &operator=(const ExpressionList &) = delete;
  ExpressionList &operator=(ExpressionList &&) noexcept;
  ExpressionList(Expression &&expr);
  ExpressionList &with_expression(Expression &&expr);
  ~ExpressionList();
};

class Array {
  ExpressionList elements;

public:
  Array() = default;
  Array(ExpressionList &&elements) : elements(std::move(elements)) {}

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_ACCESSOR(elements, ExpressionList, elements)
};

class Literal {
  std::variant<Nil, Boolean, Number, Float, String, Array> inner;

public:
  Literal() = default;
  Literal(Nil nil) : inner(nil) {}
  Literal(Boolean boolean) : inner(boolean) {}
  Literal(Number num) : inner(num) {}
  Literal(Float num) : inner(num) {}
  Literal(String &&string) : inner(std::move(string)) {}
  Literal(Array &&array) : inner(std::move(array)) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    visit([&out, depth](const auto &node) -> void { node.print(out, depth); });
  }

  VISIT(inner)
};

} // namespace l3::ast
