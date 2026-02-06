module;

#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <utils/accessor.h>
#include <utils/visit.h>
#include <variant>

export module l3.ast:last_statement;

import utils;

import :expression;

export namespace l3::ast {

class Expression;

class ReturnStatement {
  std::optional<Expression> expression;

public:
  ReturnStatement();
  ReturnStatement(Expression &&expression);

  ReturnStatement(const ReturnStatement &) = delete;
  ReturnStatement(ReturnStatement &&) noexcept;
  ReturnStatement &operator=(const ReturnStatement &) = delete;
  ReturnStatement &operator=(ReturnStatement &&) noexcept;
  ~ReturnStatement();

  utils::optional_cref<Expression> get_expression() const {
    return expression.transform(
        [](const auto &expression) -> std::reference_wrapper<const Expression> {
          return expression;
        }
    );
  }
  utils::optional_ref<Expression> get_expression_mut() {
    return expression.transform(
        [](auto &expression) -> std::reference_wrapper<Expression> {
          return expression;
        }
    );
  }
};

class BreakStatement {};

class ContinueStatement {};

class LastStatement {
  std::variant<ReturnStatement, BreakStatement, ContinueStatement> inner;

public:
  LastStatement();
  LastStatement(ReturnStatement &&statement);
  LastStatement(BreakStatement statement);
  LastStatement(ContinueStatement statement);

  VISIT(inner)
};

} // namespace l3::ast
