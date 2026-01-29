module;

#include <memory>
#include <optional>
#include <utils/accessor.h>
#include <utils/match.h>
#include <variant>

export module l3.ast:last_statement;

import :expression;

export namespace l3::ast {

class ReturnStatement {
  std::optional<Expression> expr;

public:
  ReturnStatement() = default;
  ReturnStatement(Expression &&expression) : expr(std::move(expression)) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Return");
    if (expr) {
      expr->print(out, depth + 1);
    }
  }

  DEFINE_ACCESSOR(expression, std::optional<Expression>, expr)
};

class BreakStatement {
public:
  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Break");
  }
};

class ContinueStatement {
public:
  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Continue");
  }
};

class LastStatement {
  std::variant<ReturnStatement, BreakStatement, ContinueStatement> inner;

public:
  LastStatement() = default;
  LastStatement(ReturnStatement &&statement) : inner(std::move(statement)) {}
  LastStatement(BreakStatement statement) : inner(statement) {}
  LastStatement(ContinueStatement statement) : inner(statement) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    visit([&out, depth](const auto &node) -> void {
      node.print(out, depth + 1);
    });
  }

  VISIT(inner)
};

} // namespace l3::ast
