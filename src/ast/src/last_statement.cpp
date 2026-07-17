module l3.ast;

namespace l3::ast {

ReturnStatement::ReturnStatement(location::Location location)
    : location_(location) {}
ReturnStatement::ReturnStatement(
    Expression &&expression, location::Location location
)
    : expression(std::move(expression)), location_(location) {}

ReturnStatement::ReturnStatement(ReturnStatement &&) noexcept = default;
ReturnStatement &
ReturnStatement::operator=(ReturnStatement &&) noexcept = default;
ReturnStatement::~ReturnStatement() = default;

LastStatement::LastStatement() = default;
LastStatement::LastStatement(ReturnStatement &&statement)
    : inner(std::move(statement)) {}
LastStatement::LastStatement(BreakStatement statement) : inner(statement) {}
LastStatement::LastStatement(ContinueStatement statement) : inner(statement) {}

utils::optional_cref<Expression> ReturnStatement::get_expression() const {
  return expression.transform(
      [](const auto &expr) -> std::reference_wrapper<const Expression> {
        return expr;
      }
  );
}

utils::optional_ref<Expression> ReturnStatement::get_expression_mut() {
  return expression.transform(
      [](auto &expr) -> std::reference_wrapper<Expression> { return expr; }
  );
}

const location::Location &LastStatement::get_location() const {
  return std::visit(
      [](const auto &node) -> const location::Location & {
        return node.get_location();
      },
      inner
  );
}

} // namespace l3::ast
