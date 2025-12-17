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

struct FunctionCall {
  Identifier name;
  std::vector<Expression> args;
};

using PrefixExpression =
    std::variant<Var, std::shared_ptr<Expression>, FunctionCall>;

struct UnaryExpression {
  Unary op;
  std::shared_ptr<Expression> expr;
};

class BinaryExpression {
  std::shared_ptr<Expression> lhs;
  Binary op;
  std::shared_ptr<Expression> rhs;

public:
  BinaryExpression() = default;

  BinaryExpression(Expression &&lhs, Binary op, Expression &&rhs)
      : lhs(std::make_shared<Expression>(std::move(lhs))), op(op),
        rhs(std::make_shared<Expression>(std::move(rhs))) {}

  BinaryExpression(Expression &lhs, Binary op, Expression &rhs)
      : lhs(std::make_shared<Expression>(lhs)), op(op),
        rhs(std::make_shared<Expression>(rhs)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

class Expression {
  std::variant<
      Literal,
      // UnaryExpression,
      BinaryExpression,
      Var
      // PrefixExpression,
      // AnonymousFunction,
      // Table
      >
      inner;

public:
  Expression() = default;

  Expression(Literal &&literal) : inner(std::move(literal)) {}
  // Expression(UnaryExpression &&expression) : inner(std::move(expression)) {}
  Expression(BinaryExpression &&expression) : inner(std::move(expression)) {}
  Expression(Var &&var) : inner(std::move(var)) {}
  // Expression(PrefixExpression &&expression) : inner(std::move(expression)) {}
  // Expression(AnonymousFunction &&function) : inner(std::move(function)) {}
  // Expression(Table &&table) : inner(std::move(table)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::indent(out, depth);
    std::format_to(out, "Expression\n");
    inner.visit([&out, depth](const auto &node) -> void {
      node.print(out, depth + 1);
    });
  }
};

inline void BinaryExpression::print(
    std::output_iterator<char> auto &out, size_t depth
) const {
  detail::indent(out, depth);
  std::format_to(out, "BinaryExpression\n");
  lhs->print(out, depth + 1);
  detail::indent(out, depth + 1);
  std::format_to(out, "{}\n", static_cast<uint8_t>(op));
  rhs->print(out, depth + 1);
}

} // namespace l3::ast
