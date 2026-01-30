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

} // namespace l3::ast
