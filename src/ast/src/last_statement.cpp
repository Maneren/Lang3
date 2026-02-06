module;

#include <memory>
#include <utility>

module l3.ast;

import :expression;

namespace l3::ast {

ReturnStatement::ReturnStatement() = default;
ReturnStatement::ReturnStatement(Expression &&expression)
    : expression(std::move(expression)) {}

ReturnStatement::ReturnStatement(ReturnStatement &&) noexcept = default;
ReturnStatement &
ReturnStatement::operator=(ReturnStatement &&) noexcept = default;
ReturnStatement::~ReturnStatement() = default;

LastStatement::LastStatement() = default;
LastStatement::LastStatement(ReturnStatement &&statement)
    : inner(std::move(statement)) {}
LastStatement::LastStatement(BreakStatement statement) : inner(statement) {}
LastStatement::LastStatement(ContinueStatement statement) : inner(statement) {}

} // namespace l3::ast
