#pragma once

#include <cstdint>

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
