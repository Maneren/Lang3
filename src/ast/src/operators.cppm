module;

#include <cstdint>
#include <format>

export module l3.ast:operators;

import utils;

export {
  namespace l3::ast {

  enum class UnaryOperator : std::uint8_t { Plus, Minus, Not };
  enum class BinaryOperator : std::uint8_t {
    Plus,
    Minus,
    Multiply,
    Divide,
    Modulo,
    Power,
  };

  enum class ComparisonOperator : std::uint8_t {
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
  };
  enum class LogicalOperator : std::uint8_t { And, Or };

  enum class AssignmentOperator : std::uint8_t {
    Assign,
    Plus,
    Minus,
    Multiply,
    Divide,
    Modulo,
    Power,
  };

  } // namespace l3::ast

  template <>
  struct std::formatter<l3::ast::UnaryOperator>
      : utils::static_formatter<l3::ast::UnaryOperator> {
    static constexpr auto
    format(l3::ast::UnaryOperator op, std::format_context &ctx) {
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

  template <>
  struct std::formatter<l3::ast::BinaryOperator>
      : utils::static_formatter<l3::ast::BinaryOperator> {
    static constexpr auto
    format(l3::ast::BinaryOperator op, std::format_context &ctx) {
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
      }
    }
  };

  template <>
  struct std::formatter<l3::ast::ComparisonOperator>
      : utils::static_formatter<l3::ast::ComparisonOperator> {
    static constexpr auto
    format(l3::ast::ComparisonOperator op, std::format_context &ctx) {
      using namespace l3::ast;
      switch (op) {
      case ComparisonOperator::Equal:
        return std::format_to(ctx.out(), "Equal");
      case ComparisonOperator::NotEqual:
        return std::format_to(ctx.out(), "NotEqual");
      case ComparisonOperator::Less:
        return std::format_to(ctx.out(), "Less");
      case ComparisonOperator::LessEqual:
        return std::format_to(ctx.out(), "LessEqual");
      case ComparisonOperator::Greater:
        return std::format_to(ctx.out(), "Greater");
      case ComparisonOperator::GreaterEqual:
        return std::format_to(ctx.out(), "GreaterEqual");
      }
    }
  };

  template <>
  struct std::formatter<l3::ast::LogicalOperator>
      : utils::static_formatter<l3::ast::LogicalOperator> {
    static constexpr auto
    format(l3::ast::LogicalOperator op, std::format_context &ctx) {
      using namespace l3::ast;
      switch (op) {
      case LogicalOperator::And:
        return std::format_to(ctx.out(), "And");
      case LogicalOperator::Or:
        return std::format_to(ctx.out(), "Or");
      }
    }
  };

  template <>
  struct std::formatter<l3::ast::AssignmentOperator>
      : utils::static_formatter<l3::ast::AssignmentOperator> {
    static constexpr auto
    format(l3::ast::AssignmentOperator op, std::format_context &ctx) {
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
}
