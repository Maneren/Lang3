#pragma once

#include "ast/ast.hpp"
#include <concepts>
#include <cstddef>
#include <format>
#include <iterator>
#include <string>
#include <utils/format.h>

namespace l3::ast {

namespace detail {

constexpr void indent(std::output_iterator<char> auto &out, std::size_t depth) {
  for (std::size_t i = 0; i < depth; ++i) {
    std::format_to(out, "▏ ");
  }
}

template <typename... Args>
constexpr void format_indented(
    std::output_iterator<char> auto &out,
    std::size_t depth,
    std::format_string<Args...> fmt,
    Args &&...args
) {
  indent(out, depth);
  std::format_to(out, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
constexpr void format_indented_line(
    std::output_iterator<char> auto &out,
    std::size_t depth,
    std::format_string<Args...> fmt,
    Args &&...args
) {
  indent(out, depth);
  std::format_to(out, fmt, std::forward<Args>(args)...);
  std::format_to(out, "\n");
}

} // namespace detail

inline void
String::print(std::output_iterator<char> auto &out, std::size_t depth) const {
  detail::format_indented_line(out, depth, "String \"{}\"", value);
}

inline void
Number::print(std::output_iterator<char> auto &out, std::size_t depth) const {
  detail::format_indented_line(out, depth, "Number {}", value);
}

inline void
Float::print(std::output_iterator<char> auto &out, std::size_t depth) const {
  detail::format_indented_line(out, depth, "Float {}", value);
}

inline void
Boolean::print(std::output_iterator<char> auto &out, std::size_t depth) const {
  detail::format_indented_line(out, depth, "Boolean {}", value);
}

inline void
Nil::print(std::output_iterator<char> auto &out, std::size_t depth) const {
  detail::format_indented_line(out, depth, "Nil");
}

inline void
Literal::print(std::output_iterator<char> auto &out, std::size_t depth) const {
  visit([&out, depth](const auto &node) -> void { node.print(out, depth); });
}

inline void UnaryExpression::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  detail::format_indented_line(out, depth, "UnaryExpression {}", op);
  expr->print(out, depth + 1);
}

inline void BinaryExpression::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  detail::format_indented_line(out, depth, "BinaryExpression {}", op);
  lhs->print(out, depth + 1);
  rhs->print(out, depth + 1);
}

inline void IndexExpression::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  detail::format_indented_line(out, depth, "IndexExpression");
  base->print(out, depth + 1);
  index->print(out, depth + 1);
};

inline void Identifier::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  detail::format_indented_line(out, depth, "Identifier '{}'", id);
}

inline void
Variable::print(std::output_iterator<char> auto &out, std::size_t depth) const {
  detail::format_indented_line(out, depth, "Variable '{}'", id.get_name());
}

inline void FunctionCall::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  detail::format_indented_line(out, depth, "FunctionCall");
  name.print(out, depth + 1);
  detail::format_indented_line(out, depth + 1, "Arguments");
  for (const auto &arg : args) {
    arg.print(out, depth + 2);
  }
}

inline void FunctionBody::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  detail::format_indented_line(out, depth, "Parameters");
  for (const Identifier &parameter : parameters) {
    parameter.print(out, depth + 1);
  }
  block->print(out, depth);
}

inline void NamedFunction::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  detail::format_indented_line(out, depth, "NamedFunction");
  name.print(out, depth + 1);
  body.print(out, depth + 1);
}

inline void AnonymousFunction::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  detail::format_indented_line(out, depth, "AnonymousFunction");
  body.print(out, depth + 1);
}

inline void Expression::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  visit([&out, depth](const auto &node) -> void { node.print(out, depth); });
}

inline void
IfBase::print(std::output_iterator<char> auto &out, std::size_t depth) const {
  detail::format_indented_line(out, depth, "Condition");
  condition->print(out, depth + 1);
  block->print(out, depth);
}

inline void ElseIfList::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  detail::format_indented_line(out, depth, "ElseIfList");
  for (const auto &elseif : get_elseifs()) {
    detail::format_indented_line(out, depth + 1, "ElseIf");
    elseif.print(out, depth + 2);
  }
}

inline void IfExpression::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  detail::format_indented_line(out, depth, "IfExpression");
  get_base_if().print(out, depth + 1);
  get_elseif().print(out, depth + 1);
  detail::format_indented_line(out, depth + 1, "Else");
  else_block->print(out, depth + 2);
}

inline void IfStatement::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  detail::format_indented_line(out, depth, "IfStatement");
  get_base_if().print(out, depth + 1);
  get_elseif().print(out, depth + 1);
  if (else_block) {
    detail::format_indented_line(out, depth + 1, "Else");
    else_block.value()->print(out, depth + 2);
  }
}

inline void
NameList::print(std::output_iterator<char> auto &out, std::size_t depth) const {
  for (const auto &name : *this) {
    name.print(out, depth);
  }
}

inline void OperatorAssignment::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  detail::format_indented_line(out, depth, "Assignment {}", op);
  var.print(out, depth + 1);
  expr.print(out, depth + 1);
}

inline void NameAssignment::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  detail::format_indented_line(out, depth, "Assignment");
  detail::format_indented_line(out, depth + 1, "Names");
  get_names().print(out, depth + 2);
  detail::format_indented_line(out, depth + 1, "Expression");
  get_expression().print(out, depth + 2);
}

inline void Declaration::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  detail::format_indented_line(out, depth, "Declaration");
  get_name_assignment().print(out, depth + 1);
}

inline void Statement::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  inner.visit([&out, depth](const auto &node) -> void {
    node.print(out, depth);
  });
}

inline void ContinueStatement::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  detail::format_indented_line(out, depth, "Continue");
}

inline void BreakStatement::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  detail::format_indented_line(out, depth, "Break");
}

inline void ReturnStatement::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  detail::format_indented_line(out, depth, "Return");
  if (expr) {
    expr->print(out, depth + 1);
  }
}

inline void LastStatement::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  inner.visit([&out, depth](const auto &node) -> void {
    node.print(out, depth + 1);
  });
}

inline void
Block::print(std::output_iterator<char> auto &out, std::size_t depth) const {
  detail::format_indented_line(out, depth, "Block");
  for (const Statement &statement : statements) {
    statement.print(out, depth + 1);
  }
  if (last_statement) {
    detail::format_indented_line(out, depth + 0, "LastStatement");
    last_statement->print(out, depth + 1);
  }
}

inline void
Array::print(std::output_iterator<char> auto &out, std::size_t depth) const {
  detail::format_indented_line(out, depth, "Array");
  for (const Expression &element : elements) {
    element.print(out, depth + 1);
  }
}

} // namespace l3::ast

template <typename Node>
  requires requires(
      Node const &node, std::back_insert_iterator<std::string> &out
  ) {
    { node.print(out) } -> std::same_as<void>;
  }
struct std::formatter<Node> : utils::static_formatter<Node> {
  static constexpr auto format(Node const &node, std::format_context &ctx) {
    auto out = ctx.out();
    node.print(out);
    return out;
  }
};

template <>
struct std::formatter<l3::ast::Identifier>
    : utils::static_formatter<l3::ast::Identifier> {
  static constexpr auto
  format(l3::ast::Identifier const &id, std::format_context &ctx) {
    return std::format_to(ctx.out(), "{}", id.get_name());
  }
};

template <> struct std::formatter<l3::ast::UnaryOperator> {
  static constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
  static constexpr auto format(l3::ast::UnaryOperator op, format_context &ctx) {
    using namespace l3::ast;
    switch (op) {
    case UnaryOperator::Plus:
      return std::format_to(ctx.out(), "Plus");
    case UnaryOperator::Minus:
      return std::format_to(ctx.out(), "Minus");
    case UnaryOperator::Not:
      return std::format_to(ctx.out(), "Not");
    }
  }
};

template <> struct std::formatter<l3::ast::BinaryOperator> {
  static constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
  static constexpr auto
  format(l3::ast::BinaryOperator op, format_context &ctx) {
    using namespace l3::ast;
    switch (op) {
    case BinaryOperator::Plus:
      return std::format_to(ctx.out(), "Plus");
    case BinaryOperator::Minus:
      return std::format_to(ctx.out(), "Minus");
    case BinaryOperator::Multiply:
      return std::format_to(ctx.out(), "Multiply");
    case BinaryOperator::Divide:
      return std::format_to(ctx.out(), "Divide");
    case BinaryOperator::Modulo:
      return std::format_to(ctx.out(), "Modulo");
    case BinaryOperator::Power:
      return std::format_to(ctx.out(), "Power");
    case BinaryOperator::Equal:
      return std::format_to(ctx.out(), "Equal");
    case BinaryOperator::NotEqual:
      return std::format_to(ctx.out(), "NotEqual");
    case BinaryOperator::Less:
      return std::format_to(ctx.out(), "Less");
    case BinaryOperator::LessEqual:
      return std::format_to(ctx.out(), "LessEqual");
    case BinaryOperator::Greater:
      return std::format_to(ctx.out(), "Greater");
    case BinaryOperator::GreaterEqual:
      return std::format_to(ctx.out(), "GreaterEqual");
    case BinaryOperator::And:
      return std::format_to(ctx.out(), "And");
    case BinaryOperator::Or:
      return std::format_to(ctx.out(), "Or");
    }
  }
};

template <> struct std::formatter<l3::ast::AssignmentOperator> {
  static constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
  static constexpr auto
  format(l3::ast::AssignmentOperator op, format_context &ctx) {
    using namespace l3::ast;
    switch (op) {
    case AssignmentOperator::Assign:
      return std::format_to(ctx.out(), "Assign");
    case AssignmentOperator::Plus:
      return std::format_to(ctx.out(), "Plus");
    case AssignmentOperator::Minus:
      return std::format_to(ctx.out(), "Minus");
    case AssignmentOperator::Multiply:
      return std::format_to(ctx.out(), "Multiply");
    case AssignmentOperator::Divide:
      return std::format_to(ctx.out(), "Divide");
    case AssignmentOperator::Modulo:
      return std::format_to(ctx.out(), "Modulo");
    case AssignmentOperator::Power:
      return std::format_to(ctx.out(), "Power");
    }
  }
};
