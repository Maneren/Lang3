#pragma once

#include "expression.hpp"
#include "function.hpp"
#include "identifier.hpp"
#include "if_else.hpp"
#include "operator.hpp"
#include "utils/match.h"
#include <cstddef>
#include <iterator>
#include <optional>
#include <utility>
#include <variant>

namespace l3::ast {

class OperatorAssignment {
  Variable var;
  AssignmentOperator op = AssignmentOperator::Assign;
  Expression expr;

public:
  OperatorAssignment() = default;
  OperatorAssignment(Variable &&var, AssignmentOperator op, Expression &&expr)
      : var(std::move(var)), op(op), expr(std::move(expr)) {}

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] const Variable &get_variable() const { return var; }
  Variable &get_variable_mut() { return var; }
  [[nodiscard]] AssignmentOperator get_operator() const { return op; }
  AssignmentOperator &get_operator_mut() { return op; }
  [[nodiscard]] const Expression &get_expression() const { return expr; }
  Expression &get_expression_mut() { return expr; }
};

class NameAssignment {
  NameList names;
  Expression expr;

public:
  NameAssignment() = default;
  NameAssignment(NameList &&names, Expression &&expr)
      : names(std::move(names)), expr(std::move(expr)) {}

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] const NameList &get_names() const { return names; }
  NameList &get_names_mut() { return names; }
  [[nodiscard]] const Expression &get_expression() const { return expr; }
  Expression &get_expression_mut() { return expr; }
};

using Assignment = std::variant<OperatorAssignment, NameAssignment>;

class Declaration {
  NameAssignment name_assignment;
  bool const_ = false;

public:
  Declaration() = default;
  Declaration(NameAssignment &&name_assignment, bool is_const = false)
      : name_assignment(std::move(name_assignment)), const_(is_const) {}

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] const NameList &get_names() const {
    return name_assignment.get_names();
  }
  NameList &get_names_mut() { return name_assignment.get_names_mut(); }
  [[nodiscard]] const Expression &get_expression() const {
    return name_assignment.get_expression();
  }
  Expression &get_expression_mut() {
    return name_assignment.get_expression_mut();
  }
  [[nodiscard]] const NameAssignment &get_name_assignment() const {
    return name_assignment;
  }
  NameAssignment &get_name_assignment_mut() { return name_assignment; }
  [[nodiscard]] bool is_const() const { return const_; }
};

class Statement {
  std::variant<
      OperatorAssignment,
      NameAssignment,
      Declaration,
      FunctionCall,
      IfStatement,
      NamedFunction>
      inner;

public:
  Statement() = default;
  Statement(const Statement &) = delete;
  Statement(Statement &&) = default;
  Statement &operator=(const Statement &) = delete;
  Statement &operator=(Statement &&) = default;
  ~Statement() = default;

  Statement(OperatorAssignment &&assignment) : inner(std::move(assignment)) {}
  Statement(NameAssignment &&assignment) : inner(std::move(assignment)) {}
  Statement(Assignment &&assignment) {
    match::match(std::move(assignment), [this](auto &&assignment) {
      inner = std::forward<decltype(assignment)>(assignment);
    });
  }
  Statement(Declaration &&declaration) : inner(std::move(declaration)) {}
  Statement(FunctionCall &&call) : inner(std::move(call)) {}
  Statement(IfStatement &&clause) : inner(std::move(clause)) {}
  Statement(NamedFunction &&function) : inner(std::move(function)) {}

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

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

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] const auto &get_expression() const { return expr; }
  auto &get_expression_mut() { return expr; }
};

class BreakStatement {
public:
  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;
};

class ContinueStatement {
public:
  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;
};

class LastStatement {
  std::variant<ReturnStatement, BreakStatement, ContinueStatement> inner;

public:
  LastStatement() = default;
  LastStatement(ReturnStatement &&statement) : inner(std::move(statement)) {}
  LastStatement(BreakStatement statement) : inner(statement) {}
  LastStatement(ContinueStatement statement) : inner(statement) {}

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  auto visit(auto &&visitor) const -> decltype(auto) {
    return inner.visit(visitor);
  }
  auto visit(auto &&visitor) -> decltype(auto) { return inner.visit(visitor); }
};

} // namespace l3::ast
