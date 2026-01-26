#pragma once

#include "function.hpp"
#include "identifier.hpp"
#include "if_else.hpp"
#include "literal.hpp"
#include "operator.hpp"
#include "utils/accessor.h"
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>
#include <variant>

namespace l3::ast {

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

class Expression {
  using ExpressionVariant = std::variant<
      Literal,
      UnaryExpression,
      BinaryExpression,
      Variable,
      FunctionCall,
      IndexExpression,
      AnonymousFunction,
      IfExpression
      // Table
      >;

  ExpressionVariant inner;

public:
  Expression() = default;

  Expression(Literal &&literal) : inner(std::move(literal)) {}
  Expression(UnaryExpression &&expression) : inner(std::move(expression)) {}
  Expression(BinaryExpression &&expression) : inner(std::move(expression)) {}
  Expression(Variable &&var) : inner(std::move(var)) {}
  Expression(FunctionCall &&call) : inner(std::move(call)) {}
  Expression(IndexExpression &&index) : inner(std::move(index)) {}
  Expression(AnonymousFunction &&function) : inner(std::move(function)) {}
  Expression(IfExpression &&clause) : inner(std::move(clause)) {}
  // Expression(Table &&table) : inner(std::move(table)) {}

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  auto visit(auto &&visitor) const -> decltype(auto) {
    return std::visit(visitor, inner);
  }
  auto visit(auto &&visitor) -> decltype(auto) {
    return std::visit(visitor, inner);
  }
};

} // namespace l3::ast
