#pragma once

#include "expression.hpp"
#include "function.hpp"
#include "if_else.hpp"
#include "operator.hpp"

namespace l3::ast {

class Assignment {
  Variable var;
  AssignmentOperator op;
  Expression expr;

public:
  Assignment() = default;
  Assignment(Variable &&var, AssignmentOperator op, Expression &&expr)
      : var(std::move(var)), op(op), expr(std::move(expr)) {}

  void print(std::output_iterator<char> auto &out, size_t depth = 0) const;

  [[nodiscard]] const Variable &get_variable() const { return var; }
  [[nodiscard]] AssignmentOperator get_operator() const { return op; }
  [[nodiscard]] const Expression &get_expression() const { return expr; }
};

class Declaration {
  Identifier var;
  Expression expr;

public:
  Declaration() = default;
  Declaration(Identifier &&var, Expression &&expr)
      : var(std::move(var)), expr(std::move(expr)) {}

  void print(std::output_iterator<char> auto &out, size_t depth = 0) const;

  [[nodiscard]] const Identifier &get_variable() const { return var; }
  [[nodiscard]] const Expression &get_expression() const { return expr; }
};

class Statement {
  std::
      variant<Assignment, Declaration, FunctionCall, IfStatement, NamedFunction>
          inner;

public:
  Statement() = default;
  Statement(const Statement &) = delete;
  Statement(Statement &&) = default;
  Statement &operator=(const Statement &) = delete;
  Statement &operator=(Statement &&) = default;
  ~Statement() = default;

  Statement(Assignment &&assignment) : inner(std::move(assignment)) {}
  Statement(Declaration &&declaration) : inner(std::move(declaration)) {}
  Statement(FunctionCall &&call) : inner(std::move(call)) {}
  Statement(IfStatement &&clause) : inner(std::move(clause)) {}
  Statement(NamedFunction &&function) : inner(std::move(function)) {}

  void print(std::output_iterator<char> auto &out, size_t depth = 0) const;

  auto visit(auto &&visitor) const -> decltype(auto) {
    return inner.visit(visitor);
  }
  auto visit(auto &&visitor) -> decltype(auto) { return inner.visit(visitor); }
};

class ReturnStatement {
  std::optional<Expression> expr;

public:
  ReturnStatement() = default;
  ReturnStatement(Expression &&expression) : expr(std::move(expression)) {}

  void print(std::output_iterator<char> auto &out, size_t depth = 0) const;

  [[nodiscard]] const auto &get_expression() const { return expr; }
};

struct BreakStatement {
  void print(std::output_iterator<char> auto &out, size_t depth = 0) const;
};

struct ContinueStatement {
  void print(std::output_iterator<char> auto &out, size_t depth = 0) const;
};

class LastStatement {
  std::variant<ReturnStatement, BreakStatement, ContinueStatement> inner;

public:
  LastStatement() = default;
  LastStatement(ReturnStatement &&statement) : inner(std::move(statement)) {}
  LastStatement(BreakStatement statement) : inner(statement) {}
  LastStatement(ContinueStatement statement) : inner(statement) {}

  void print(std::output_iterator<char> auto &out, size_t depth = 0) const;

  auto visit(auto &&visitor) const -> decltype(auto) {
    return inner.visit(visitor);
  }
  auto visit(auto &&visitor) -> decltype(auto) { return inner.visit(visitor); }
};

} // namespace l3::ast
