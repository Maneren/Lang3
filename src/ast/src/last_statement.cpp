module;

#include <memory>
#include <utility>

module l3.ast;

import :expression;

namespace l3::ast {

ReturnStatement::ReturnStatement(Expression &&expression)
    : expression(std::make_unique<Expression>(std::move(expression))) {}

ReturnStatement::ReturnStatement(ReturnStatement &&) noexcept = default;
ReturnStatement &
ReturnStatement::operator=(ReturnStatement &&) noexcept = default;
ReturnStatement::~ReturnStatement() = default;

} // namespace l3::ast
