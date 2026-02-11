module l3.ast;

import :assignment;

namespace l3::ast {

Statement::Statement() = default;
Statement::Statement(Statement &&) noexcept = default;
Statement &Statement::operator=(Statement &&) noexcept = default;
Statement::~Statement() = default;

Statement::Statement(Assignment &&assignment) {
  match::match(std::move(assignment), [this](auto &&assignment) {
    inner = std::forward<decltype(assignment)>(assignment);
  });
}

Statement::Statement(Declaration &&declaration)
    : inner(std::move(declaration)) {}
Statement::Statement(ForLoop &&loop) : inner(std::move(loop)) {}
Statement::Statement(FunctionCall &&call) : inner(std::move(call)) {}
Statement::Statement(IfStatement &&clause) : inner(std::move(clause)) {}
Statement::Statement(NameAssignment &&assignment)
    : inner(std::move(assignment)) {}
Statement::Statement(NamedFunction &&function) : inner(std::move(function)) {}
Statement::Statement(OperatorAssignment &&assignment)
    : inner(std::move(assignment)) {}
Statement::Statement(RangeForLoop &&loop) : inner(std::move(loop)) {}
Statement::Statement(While &&loop) : inner(std::move(loop)) {}

const location::Location &Statement::get_location() const {
  return std::visit(
      [](const auto &node) -> const location::Location & {
        return node.get_location();
      },
      inner
  );
}

} // namespace l3::ast
