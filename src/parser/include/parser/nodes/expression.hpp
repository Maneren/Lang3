#pragma once

#include "detail.hpp"
#include "identifier.hpp"
#include "literal.hpp"
#include "operator.hpp"
#include <memory>
#include <variant>
#include <vector>

namespace l3::ast {

class Expression;

using ExpressionList = std::vector<Expression>;

class FunctionCall {
  Identifier name;
  std::vector<Expression> args;

public:
  FunctionCall() = default;
  FunctionCall(Identifier &&name, ExpressionList &&args);

  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

using PrefixExpression =
    std::variant<Variable, std::shared_ptr<Expression>, FunctionCall>;

class UnaryExpression {
  UnaryOperator op;
  std::shared_ptr<Expression> expr;

public:
  UnaryExpression() = default;

  UnaryExpression(UnaryOperator op, Expression &&expr)
      : op(op), expr(std::make_shared<Expression>(std::move(expr))) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

class BinaryExpression {
  std::shared_ptr<Expression> lhs;
  BinaryOperator op;
  std::shared_ptr<Expression> rhs;

public:
  BinaryExpression() = default;

  BinaryExpression(Expression &&lhs, BinaryOperator op, Expression &&rhs)
      : lhs(std::make_shared<Expression>(std::move(lhs))), op(op),
        rhs(std::make_shared<Expression>(std::move(rhs))) {}

  BinaryExpression(Expression &lhs, BinaryOperator op, Expression &rhs)
      : lhs(std::make_shared<Expression>(lhs)), op(op),
        rhs(std::make_shared<Expression>(rhs)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

using ExpressionVariant = std::variant<
    Literal,
    UnaryExpression,
    BinaryExpression,
    Variable,
    FunctionCall
    // PrefixExpression,
    // AnonymousFunction,
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
  // Expression(AnonymousFunction &&function) : inner(std::move(function)) {}
  // Expression(Table &&table) : inner(std::move(table)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    visit([&out, depth](const auto &node) -> void { node.print(out, depth); });
  }
};

inline void UnaryExpression::print(
    std::output_iterator<char> auto &out, size_t depth
) const {
  detail::format_indented_line(out, depth, "UnaryExpression {}", op);
  expr->print(out, depth + 1);
}

inline void BinaryExpression::print(
    std::output_iterator<char> auto &out, size_t depth
) const {
  detail::format_indented_line(out, depth, "BinaryExpression {}", op);
  lhs->print(out, depth + 1);
  rhs->print(out, depth + 1);
}

inline void
FunctionCall::print(std::output_iterator<char> auto &out, size_t depth) const {
  detail::format_indented_line(out, depth, "FunctionCall");
  name.print(out, depth + 1);
  for (const auto &arg : args) {
    arg.print(out, depth + 1);
  }
}

inline FunctionCall::FunctionCall(Identifier &&name, ExpressionList &&args)
    : name(std::move(name)), args(std::move(args)) {}

} // namespace l3::ast
