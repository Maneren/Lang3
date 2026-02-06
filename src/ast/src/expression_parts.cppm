module;

#include <cstdint>
#include <memory>
#include <utility>
#include <utils/accessor.h>
#include <vector>

export module l3.ast:expression_parts;

import :identifier;
import :operators;

export namespace l3::ast {

class Expression;

class UnaryExpression {
  UnaryOperator op = UnaryOperator::Plus;
  std::unique_ptr<Expression> expression;

public:
  UnaryExpression() = default;
  UnaryExpression(UnaryOperator op, Expression &&expression);

  DEFINE_VALUE_ACCESSOR_X(op)
  DEFINE_PTR_ACCESSOR_X(expression)
};

class BinaryExpression {
  std::unique_ptr<Expression> lhs;
  BinaryOperator op = BinaryOperator::Plus;
  std::unique_ptr<Expression> rhs;

public:
  BinaryExpression() = default;
  BinaryExpression(Expression &&lhs, BinaryOperator op, Expression &&rhs);

  DEFINE_PTR_ACCESSOR_X(lhs)
  DEFINE_VALUE_ACCESSOR_X(op)
  DEFINE_PTR_ACCESSOR_X(rhs)
};

class LogicalExpression {
  std::unique_ptr<Expression> lhs;
  LogicalOperator op = LogicalOperator::And;
  std::unique_ptr<Expression> rhs;

public:
  LogicalExpression() = default;
  LogicalExpression(Expression &&lhs, LogicalOperator op, Expression &&rhs);

  DEFINE_PTR_ACCESSOR_X(lhs)
  DEFINE_VALUE_ACCESSOR_X(op)
  DEFINE_PTR_ACCESSOR_X(rhs)
};

class Comparison {
  enum class Type : std::uint8_t { Equality, Inequality };

  constexpr static Type get_type(ComparisonOperator op) {
    switch (op) {
    case ComparisonOperator::Equal:
    case ComparisonOperator::NotEqual:
      return Type::Equality;
    default:
      return Type::Inequality;
    }
  }

  std::unique_ptr<Expression> start;
  std::vector<std::pair<ComparisonOperator, Expression>> comparisons;
  Type type = Type::Equality;

public:
  Comparison() = default;
  Comparison(Expression &&left, ComparisonOperator op, Expression &&right);
  bool add_comparison(ComparisonOperator op, Expression &&right);

  DEFINE_PTR_ACCESSOR_X(start)
  DEFINE_ACCESSOR_X(comparisons)
};

} // namespace l3::ast
