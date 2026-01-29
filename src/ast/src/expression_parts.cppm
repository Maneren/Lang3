module;

#include <memory>
#include <utils/accessor.h>

export module l3.ast:expression_parts;

import :identifier;
import :literal;
import :operators;

export namespace l3::ast {

class Expression;

class UnaryExpression {
  UnaryOperator op = UnaryOperator::Plus;
  std::unique_ptr<Expression> expr;

public:
  UnaryExpression() = default;
  UnaryExpression(UnaryOperator op, Expression &&expr);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_VALUE_ACCESSOR(op, UnaryOperator, op)
  DEFINE_PTR_ACCESSOR(expr, Expression, expr)
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

class IndexExpression {
  std::unique_ptr<Expression> base;
  std::unique_ptr<Expression> index;

public:
  IndexExpression();
  IndexExpression(Expression &&base, Expression &&index);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_PTR_ACCESSOR(base, Expression, base)
  DEFINE_PTR_ACCESSOR(index, Expression, index)
};

class FunctionCall {
  Variable name;
  ExpressionList args;

public:
  FunctionCall() = default;
  FunctionCall(Variable &&name, ExpressionList &&args)
      : name(std::move(name)), args(std::move(args)) {}

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_ACCESSOR(name, Variable, name)
  DEFINE_ACCESSOR(arguments, ExpressionList, args)
};

} // namespace l3::ast
