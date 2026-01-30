module;

#include <memory>
#include <optional>
#include <utils/accessor.h>
#include <utils/visit.h>
#include <variant>

export module l3.ast:last_statement;

import utils;
import :printing;

export namespace l3::ast {

class Expression;

class ReturnStatement {
  std::optional<std::unique_ptr<Expression>> expression;

public:
  ReturnStatement() = default;
  ReturnStatement(Expression &&expression);

  ReturnStatement(const ReturnStatement &) = delete;
  ReturnStatement(ReturnStatement &&) noexcept;
  ReturnStatement &operator=(const ReturnStatement &) = delete;
  ReturnStatement &operator=(ReturnStatement &&) noexcept;
  ~ReturnStatement();

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] utils::optional_cref<Expression> get_expression() const {
    return expression.transform(
        [](auto &ptr) -> std::reference_wrapper<const Expression> {
          return *ptr;
        }
    );
  }
  [[nodiscard]] utils::optional_ref<Expression> get_expression_mut() {
    return expression.transform(
        [](auto &ptr) -> std::reference_wrapper<Expression> { return *ptr; }
    );
  }
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
