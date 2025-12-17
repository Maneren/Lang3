#pragma once

#include "detail.hpp"
#include "expression.hpp"

namespace l3::ast {

struct Assignment {
  Variable var;
  Expression expr;

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::format_indented_line(out, depth, "Assignment");
    var.print(out, depth + 1);
    expr.print(out, depth + 1);
  }
};

class Declaration {
  Identifier var;
  Expression expr;

public:
  Declaration() = default;
  Declaration(Identifier &&var, Expression &&expr)
      : var(std::move(var)), expr(std::move(expr)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::format_indented_line(out, depth, "Declaration");
    var.print(out, depth + 1);
    expr.print(out, depth + 1);
  }
};

class Statement {
  std::variant<
      // Assignment,
      Declaration,
      FunctionCall
      // IfClause,
      // NamedFunction
      >
      inner;

public:
  Statement() = default;
  Statement(Declaration &&declaration) : inner(std::move(declaration)) {}
  Statement(FunctionCall &&call) : inner(std::move(call)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::format_indented_line(out, depth, "Statement");
    inner.visit([&out, depth](const auto &node) -> void {
      node.print(out, depth + 1);
    });
  }
};

class ReturnStatement {
  std::optional<Expression> expr;

public:
  ReturnStatement() = default;
  ReturnStatement(Expression &&expression) : expr(std::move(expression)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::format_indented_line(out, depth, "Return");
    if (expr) {
      expr->print(out, depth + 1);
    }
  }
};

struct BreakStatement {
  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::format_indented_line(out, depth, "Break");
  }
};

struct ContinueStatement {
  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::format_indented_line(out, depth, "Continue");
  }
};

class LastStatement {
  std::variant<ReturnStatement, BreakStatement, ContinueStatement> inner;

public:
  LastStatement() = default;
  LastStatement(ReturnStatement &&statement) : inner(std::move(statement)) {}
  LastStatement(BreakStatement statement) : inner(statement) {}
  LastStatement(ContinueStatement statement) : inner(statement) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::format_indented_line(out, depth, "LastStatement");
    inner.visit([&out, depth](const auto &node) -> void {
      node.print(out, depth + 1);
    });
  }
};

} // namespace l3::ast
