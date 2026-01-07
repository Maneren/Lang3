#pragma once

#include "nodes/block.hpp"
#include "nodes/expression.hpp"
#include "nodes/function.hpp"
#include <format>
#include <iterator>

namespace l3::ast {

namespace detail {

constexpr void indent(std::output_iterator<char> auto &out, size_t depth) {
  for (size_t i = 0; i < depth; ++i) {
    std::format_to(out, "▏ ");
  }
}

template <typename... Args>
constexpr void format_indented(
    std::output_iterator<char> auto &out,
    size_t depth,
    std::format_string<Args...> fmt,
    Args &&...args
) {
  indent(out, depth);
  std::format_to(out, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
constexpr void format_indented_line(
    std::output_iterator<char> auto &out,
    size_t depth,
    std::format_string<Args...> fmt,
    Args &&...args
) {
  indent(out, depth);
  std::format_to(out, fmt, std::forward<Args>(args)...);
  std::format_to(out, "\n");
}

} // namespace detail

inline void
String::print(std::output_iterator<char> auto &out, size_t depth) const {
  detail::format_indented_line(out, depth, "String \"{}\"", value);
}

inline void
Number::print(std::output_iterator<char> auto &out, size_t depth) const {
  detail::format_indented_line(out, depth, "Number {}", value);
}

inline void
Float::print(std::output_iterator<char> auto &out, size_t depth) const {
  detail::format_indented_line(out, depth, "Float {}", value);
}

inline void
Boolean::print(std::output_iterator<char> auto &out, size_t depth) const {
  detail::format_indented_line(out, depth, "Boolean {}", value);
}

inline void
Nil::print(std::output_iterator<char> auto &out, size_t depth) const {
  detail::format_indented_line(out, depth, "Nil");
}

inline void UnaryExpression::print(
    std::output_iterator<char> auto &out, size_t depth
) const {
  detail::format_indented_line(out, depth, "UnaryExpression {}", op);
  expr->print(out, depth + 1);
}

inline void BinaryExpression::print(
    std::output_iterator<char> auto &out, size_t depth
) const {
  detail::format_indented_line(out, depth, "BinaryExpression {}", op);
  lhs->print(out, depth + 1);
  rhs->print(out, depth + 1);
}

inline void
Identifier::print(std::output_iterator<char> auto &out, size_t depth) const {
  detail::format_indented_line(out, depth, "Identifier '{}'", id);
}

inline void
Variable::print(std::output_iterator<char> auto &out, size_t depth) const {
  detail::format_indented_line(out, depth, "Variable '{}'", id.name());
}

inline void
FunctionCall::print(std::output_iterator<char> auto &out, size_t depth) const {
  detail::format_indented_line(out, depth, "FunctionCall");
  name.print(out, depth + 1);
  detail::format_indented_line(out, depth + 1, "Arguments");
  for (const auto &arg : args) {
    arg.print(out, depth + 2);
  }
}

inline void
FunctionBody::print(std::output_iterator<char> auto &out, size_t depth) const {
  detail::format_indented_line(out, depth, "Parameters");
  for (const Identifier &parameter : parameters) {
    parameter.print(out, depth + 1);
  }
  block->print(out, depth);
}

inline void
NamedFunction::print(std::output_iterator<char> auto &out, size_t depth) const {
  detail::format_indented_line(out, depth, "NamedFunction");
  name.print(out, depth + 1);
  body.print(out, depth + 1);
}

inline void AnonymousFunction::print(
    std::output_iterator<char> auto &out, size_t depth
) const {
  detail::format_indented_line(out, depth, "AnonymousFunction");
  body.print(out, depth + 1);
}

inline void
IfBase::print(std::output_iterator<char> auto &out, size_t depth) const {
  detail::format_indented_line(out, depth, "Condition");
  condition->print(out, depth + 1);
  block->print(out, depth);
}

inline void
IfExpression::print(std::output_iterator<char> auto &out, size_t depth) const {
  detail::format_indented_line(out, depth, "IfExpression");
  get_base_if().print(out, depth + 1);
  for (const auto &elseif : get_elseif()) {
    detail::format_indented_line(out, depth + 1, "ElseIf");
    elseif.print(out, depth + 2);
  }
  detail::format_indented_line(out, depth + 1, "Else");
  else_block->print(out, depth + 2);
}

inline void
IfStatement::print(std::output_iterator<char> auto &out, size_t depth) const {
  detail::format_indented_line(out, depth, "IfStatement");
  get_base_if().print(out, depth + 1);
  for (const auto &elseif : get_elseif()) {
    detail::format_indented_line(out, depth + 1, "ElseIf");
    elseif.print(out, depth + 2);
  }
  if (else_block) {
    detail::format_indented_line(out, depth + 1, "Else");
    else_block.value()->print(out, depth + 2);
  }
}

inline void
Assignment::print(std::output_iterator<char> auto &out, size_t depth) const {
  detail::format_indented_line(out, depth, "Assignment {}", op);
  var.print(out, depth + 1);
  expr.print(out, depth + 1);
}

inline void
Declaration::print(std::output_iterator<char> auto &out, size_t depth) const {
  detail::format_indented_line(out, depth, "Declaration");
  var.print(out, depth + 1);
  expr.print(out, depth + 1);
}

inline void
Statement::print(std::output_iterator<char> auto &out, size_t depth) const {
  inner.visit([&out, depth](const auto &node) -> void {
    node.print(out, depth);
  });
}

inline void ContinueStatement::print(
    std::output_iterator<char> auto &out, size_t depth
) const {
  detail::format_indented_line(out, depth, "Continue");
}

inline void BreakStatement::print(
    std::output_iterator<char> auto &out, size_t depth
) const {
  detail::format_indented_line(out, depth, "Break");
}

inline void ReturnStatement::print(
    std::output_iterator<char> auto &out, size_t depth
) const {
  detail::format_indented_line(out, depth, "Return");
  if (expr) {
    expr->print(out, depth + 1);
  }
}

inline void
LastStatement::print(std::output_iterator<char> auto &out, size_t depth) const {
  inner.visit([&out, depth](const auto &node) -> void {
    node.print(out, depth + 1);
  });
}

inline void
Block::print(std::output_iterator<char> auto &out, size_t depth) const {
  detail::format_indented_line(out, depth, "Block");
  for (const Statement &statement : statements) {
    statement.print(out, depth + 1);
  }
  if (lastStatement) {
    detail::format_indented_line(out, depth + 0, "LastStatement");
    lastStatement->print(out, depth + 1);
  }
}

} // namespace l3::ast

template <typename Node>
  requires requires(
      Node const &node, std::back_insert_iterator<std::string> &out
  ) {
    { node.print(out) } -> std::same_as<void>;
  }
struct std::formatter<Node> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }
  static constexpr auto format(Node const &node, std::format_context &ctx) {
    auto out = ctx.out();
    node.print(out);
    return out;
  }
};
