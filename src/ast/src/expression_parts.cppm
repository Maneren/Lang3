module;

#include <memory>
#include <utils/accessor.h>

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

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_VALUE_ACCESSOR(op, UnaryOperator, op)
  DEFINE_PTR_ACCESSOR(expression, Expression, expression)
};

class BinaryExpression {
  std::unique_ptr<Expression> lhs;
  BinaryOperator op = BinaryOperator::Plus;
  std::unique_ptr<Expression> rhs;

public:
  BinaryExpression() = default;
  BinaryExpression(Expression &&lhs, BinaryOperator op, Expression &&rhs);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_PTR_ACCESSOR(lhs, Expression, lhs)
  DEFINE_PTR_ACCESSOR(rhs, Expression, rhs)
  DEFINE_VALUE_ACCESSOR(op, BinaryOperator, op)
};

class LogicalExpression {
  std::unique_ptr<Expression> lhs;
  LogicalOperator op = LogicalOperator::And;
  std::unique_ptr<Expression> rhs;

public:
  LogicalExpression() = default;
  LogicalExpression(Expression &&lhs, LogicalOperator op, Expression &&rhs);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_PTR_ACCESSOR(lhs, Expression, lhs)
  DEFINE_PTR_ACCESSOR(rhs, Expression, rhs)
  DEFINE_VALUE_ACCESSOR(op, LogicalOperator, op)
};

} // namespace l3::ast
