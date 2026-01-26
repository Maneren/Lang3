#pragma once

#include <cstdint>

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
