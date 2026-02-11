export module l3.ast:last_statement;

import utils;

import :expression;
import l3.location;

export namespace l3::ast {

class Expression;

class ReturnStatement {
  std::optional<Expression> expression;

  DEFINE_LOCATION_FIELD()

public:
  ReturnStatement(location::Location location = {});
  ReturnStatement(Expression &&expression, location::Location location = {});

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

struct BreakStatement {
  DEFINE_LOCATION_FIELD()

  constexpr BreakStatement(location::Location location = {})
      : location_(std::move(location)) {}
};

struct ContinueStatement {
  DEFINE_LOCATION_FIELD()

  constexpr ContinueStatement(location::Location location = {})
      : location_(std::move(location)) {}
};

class LastStatement {
  std::variant<ReturnStatement, BreakStatement, ContinueStatement> inner;

public:
  LastStatement();
  LastStatement(ReturnStatement &&statement);
  LastStatement(BreakStatement statement);
  LastStatement(ContinueStatement statement);

  VISIT(inner)

  [[nodiscard]] const location::Location &get_location() const;
};

} // namespace l3::ast
