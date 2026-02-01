module;

#include <memory>
#include <optional>
#include <utils/accessor.h>
#include <utils/visit.h>
#include <variant>

export module l3.ast:last_statement;

import utils;

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
};

class ContinueStatement {
public:
};

class LastStatement {
  std::variant<ReturnStatement, BreakStatement, ContinueStatement> inner;

public:
  LastStatement() = default;
  LastStatement(ReturnStatement &&statement) : inner(std::move(statement)) {}
  LastStatement(BreakStatement statement) : inner(statement) {}
  LastStatement(ContinueStatement statement) : inner(statement) {}

  VISIT(inner)
};

} // namespace l3::ast
