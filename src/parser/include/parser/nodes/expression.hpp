#pragma once

#include "function.hpp"
#include "identifier.hpp"
#include "literal.hpp"
#include "operator.hpp"
#include <memory>
#include <variant>

namespace l3::ast {

class Expression;

using PrefixExpression =
    std::variant<Variable, std::unique_ptr<Expression>, FunctionCall>;

class UnaryExpression {
  UnaryOperator op;
  std::unique_ptr<Expression> expr;

public:
  UnaryExpression() = default;
  UnaryExpression(UnaryOperator op, Expression &&expr);

  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

class BinaryExpression {
  std::unique_ptr<Expression> lhs;
  BinaryOperator op;
  std::unique_ptr<Expression> rhs;

public:
  BinaryExpression() = default;
  BinaryExpression(Expression &&lhs, BinaryOperator op, Expression &&rhs);

  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

using ExpressionVariant = std::variant<
    Literal,
    UnaryExpression,
    BinaryExpression,
    Variable,
    FunctionCall,
    // PrefixExpression,
    AnonymousFunction
    // Table
    >;

class Expression : ExpressionVariant {
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
  // Expression(Table &&table) : inner(std::move(table)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    visit([&out, depth](const auto &node) -> void { node.print(out, depth); });
  }
};

} // namespace l3::ast
