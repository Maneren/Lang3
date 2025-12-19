#pragma once

#include <cstdint>
#include <format>

namespace l3::ast {

enum class UnaryOperator : std::uint8_t { Plus, Minus, Not };
enum class BinaryOperator : std::uint8_t {
  Plus,
  Minus,
  Multiply,
  Divide,
  Modulo,
  Power,
  Equal,
  NotEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  And,
  Or
};

enum class AssignmentOperator : std::uint8_t {
  Assign,
  Plus,
  Minus,
  Multiply,
  Divide,
  Modulo,
  Power
};

} // namespace l3::ast

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
