#pragma once

#include "detail.hpp"
#include "expression.hpp"

namespace l3::ast {

struct Assignment {
  Var var;
  Expression expr;

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::indent(out, depth);
    std::format_to(out, "Assignment '{}'\n", var);
    expr.print(out, depth + 1);
  }
};

struct Declaration {
  Identifier var;
  Expression expr;

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::indent(out, depth);
    std::format_to(out, "Declaration '{}'\n", var);
    expr.print(out, depth + 1);
  }
};

class Statement {
  std::variant<
      // Assignment,
      Declaration,
      Expression
      // FunctionCall,
      // IfClause,
      // NamedFunction
      >
      inner;

public:
  Statement() = default;
  Statement(Declaration &&declaration) : inner(std::move(declaration)) {}
  Statement(Expression &&expression) : inner(std::move(expression)) {}
  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::indent(out, depth);
    std::format_to(out, "Statement\n");
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
    detail::indent(out, depth);
    std::format_to(out, "Return\n");
    if (expr) {
      expr->print(out, depth + 1);
    }
  }
};

struct BreakStatement {
  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::indent(out, depth);
    std::format_to(out, "Break\n");
  }
};

struct ContinueStatement {
  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::indent(out, depth);
    std::format_to(out, "Continue\n");
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
    detail::indent(out, depth);
    std::format_to(out, "LastStatement\n");
    inner.visit([&out, depth](const auto &node) -> void {
      node.print(out, depth + 1);
    });
  }
};

} // namespace l3::ast
