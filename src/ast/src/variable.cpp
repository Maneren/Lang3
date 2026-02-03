module;

#include <memory>
#include <utility>

module l3.ast;

namespace l3::ast {

IndexExpression::IndexExpression() = default;
IndexExpression::IndexExpression(Variable &&base, Expression &&index)
    : base(std::make_unique<Variable>(std::move(base))),
      index(std::make_unique<Expression>(std::move(index))) {};

IndexExpression::IndexExpression(IndexExpression &&) noexcept = default;
IndexExpression &
IndexExpression::operator=(IndexExpression &&) noexcept = default;
IndexExpression::~IndexExpression() = default;

Variable::Variable() = default;
Variable::Variable(Identifier &&id) : inner(std::move(id)) {}
Variable::Variable(IndexExpression &&ie) : inner(std::move(ie)) {}

} // namespace l3::ast
