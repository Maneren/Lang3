#pragma once

#include "function.hpp"
#include "identifier.hpp"
#include "if_else.hpp"
#include "literal.hpp"
#include "operator.hpp"
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

  [[nodiscard]] UnaryOperator get_op() const { return op; }
  [[nodiscard]] UnaryOperator &get_op_mut() { return op; }
  [[nodiscard]] const auto &get_expr() const { return *expr; }
  [[nodiscard]] auto &get_expr_mut() { return *expr; }
};

class BinaryExpression {
  std::unique_ptr<Expression> lhs;
  BinaryOperator op = BinaryOperator::Plus;
  std::unique_ptr<Expression> rhs;

public:
  BinaryExpression() = default;
  BinaryExpression(Expression &&lhs, BinaryOperator op, Expression &&rhs);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] const auto &get_lhs() const { return *lhs; }
  [[nodiscard]] auto &get_lhs_mut() { return *lhs; }
  [[nodiscard]] const auto &get_rhs() const { return *rhs; }
  [[nodiscard]] auto &get_rhs_mut() { return *rhs; }
  [[nodiscard]] BinaryOperator get_op() const { return op; }
  [[nodiscard]] BinaryOperator &get_op_mut() { return op; }
};

class IndexExpression {
  std::unique_ptr<Expression> base;
  std::unique_ptr<Expression> index;

public:
  IndexExpression();
  IndexExpression(Expression &&base, Expression &&index);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] const auto &get_base() const { return *base; }
  [[nodiscard]] auto &get_base_mut() { return *base; }
  [[nodiscard]] const auto &get_index() const { return *index; }
  [[nodiscard]] auto &get_index_mut() { return *index; }
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
