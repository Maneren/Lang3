#pragma once

#include <cstdint>
#include <format>

namespace l3::ast {

enum class Unary : std::uint8_t { Plus, Minus, Not };
enum class Binary : std::uint8_t {
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

} // namespace l3::ast

template <> struct std::formatter<l3::ast::Unary> {
  static constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
  static constexpr auto format(l3::ast::Unary op, format_context &ctx) {
    switch (op) {
    case l3::ast::Unary::Plus:
      return std::format_to(ctx.out(), "Plus");
    case l3::ast::Unary::Minus:
      return std::format_to(ctx.out(), "Minus");
    case l3::ast::Unary::Not:
      return std::format_to(ctx.out(), "Not");
    }
  }
};

template <> struct std::formatter<l3::ast::Binary> {
  static constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
  static constexpr auto format(l3::ast::Binary op, format_context &ctx) {
    switch (op) {
    case l3::ast::Binary::Plus:
      return std::format_to(ctx.out(), "Plus");
    case l3::ast::Binary::Minus:
      return std::format_to(ctx.out(), "Minus");
    case l3::ast::Binary::Multiply:
      return std::format_to(ctx.out(), "Multiply");
    case l3::ast::Binary::Divide:
      return std::format_to(ctx.out(), "Divide");
    case l3::ast::Binary::Modulo:
      return std::format_to(ctx.out(), "Modulo");
    case l3::ast::Binary::Power:
      return std::format_to(ctx.out(), "Power");
    case l3::ast::Binary::Equal:
      return std::format_to(ctx.out(), "Equal");
    case l3::ast::Binary::NotEqual:
      return std::format_to(ctx.out(), "NotEqual");
    case l3::ast::Binary::Less:
      return std::format_to(ctx.out(), "Less");
    case l3::ast::Binary::LessEqual:
      return std::format_to(ctx.out(), "LessEqual");
    case l3::ast::Binary::Greater:
      return std::format_to(ctx.out(), "Greater");
    case l3::ast::Binary::GreaterEqual:
      return std::format_to(ctx.out(), "GreaterEqual");
    case l3::ast::Binary::And:
      return std::format_to(ctx.out(), "And");
    case l3::ast::Binary::Or:
      return std::format_to(ctx.out(), "Or");
    }
  }
};
