#pragma once

#include "function.hpp"
#include "identifier.hpp"
#include "if_else.hpp"
#include "literal.hpp"
#include "operator.hpp"
#include <memory>
#include <variant>

namespace l3::ast {

class Expression;

using PrefixExpression =
    std::variant<Variable, std::unique_ptr<Expression>, FunctionCall>;

class UnaryExpression {
  UnaryOperator op = UnaryOperator::Plus;
  std::unique_ptr<Expression> expr;

public:
  UnaryExpression() = default;
  UnaryExpression(UnaryOperator op, Expression &&expr);

  void print(std::output_iterator<char> auto &out, size_t depth = 0) const;
};

class BinaryExpression {
  std::unique_ptr<Expression> lhs;
  BinaryOperator op = BinaryOperator::Plus;
  std::unique_ptr<Expression> rhs;

public:
  BinaryExpression() = default;
  BinaryExpression(Expression &&lhs, BinaryOperator op, Expression &&rhs);

  void print(std::output_iterator<char> auto &out, size_t depth = 0) const;

  [[nodiscard]] const auto &get_lhs() const { return *lhs; }
  [[nodiscard]] const auto &get_rhs() const { return *rhs; }

  auto &get_lhs() { return *lhs; }
  auto &get_rhs() { return *rhs; }

  [[nodiscard]] BinaryOperator get_op() const { return op; }
};

using ExpressionVariant = std::variant<
    Literal,
    UnaryExpression,
    BinaryExpression,
    Variable,
    FunctionCall,
    // PrefixExpression,
    AnonymousFunction,
    IfExpression
    // Table
    >;

class Expression : public ExpressionVariant {
public:
  Expression() = default;

  Expression(Literal &&literal) : ExpressionVariant(std::move(literal)) {}
  Expression(UnaryExpression &&expression)
      : ExpressionVariant(std::move(expression)) {}
  Expression(BinaryExpression &&expression)
      : ExpressionVariant(std::move(expression)) {}
  Expression(Variable &&var) : ExpressionVariant(std::move(var)) {}
  Expression(FunctionCall &&call) : ExpressionVariant(std::move(call)) {}
  // Expression(PrefixExpression &&expression) : inner(std::move(expression)) {}
  Expression(AnonymousFunction &&function)
      : ExpressionVariant(std::move(function)) {}
  Expression(IfExpression &&clause) : ExpressionVariant(std::move(clause)) {}
  // Expression(Table &&table) : inner(std::move(table)) {}

  void print(std::output_iterator<char> auto &out, size_t depth = 0) const {
    visit([&out, depth](const auto &node) -> void { node.print(out, depth); });
  }
};

} // namespace l3::ast
