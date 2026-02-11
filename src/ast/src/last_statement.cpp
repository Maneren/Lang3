module l3.ast;

import :expression;

namespace l3::ast {

ReturnStatement::ReturnStatement(location::Location location)
    : location_(std::move(location)) {}
ReturnStatement::ReturnStatement(
    Expression &&expression, location::Location location
)
    : expression(std::move(expression)), location_(std::move(location)) {}

ReturnStatement::ReturnStatement(ReturnStatement &&) noexcept = default;
ReturnStatement &
ReturnStatement::operator=(ReturnStatement &&) noexcept = default;
ReturnStatement::~ReturnStatement() = default;

LastStatement::LastStatement() = default;
LastStatement::LastStatement(ReturnStatement &&statement)
    : inner(std::move(statement)) {}
LastStatement::LastStatement(BreakStatement statement) : inner(statement) {}
LastStatement::LastStatement(ContinueStatement statement) : inner(statement) {}

const location::Location &LastStatement::get_location() const {
  return std::visit(
      [](const auto &node) -> const location::Location & {
        return node.get_location();
      },
      inner
  );
}

} // namespace l3::ast
