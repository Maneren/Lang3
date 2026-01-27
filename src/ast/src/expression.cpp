module;

#include <memory>

module l3.ast;

namespace l3::ast {

IndexExpression::IndexExpression() = default;
IndexExpression::IndexExpression(Expression &&base, Expression &&index)
    : base(std::make_unique<Expression>(std::move(base))),
      index(std::make_unique<Expression>(std::move(index))) {};

UnaryExpression::UnaryExpression(UnaryOperator op, Expression &&expr)
    : op(op), expr(std::make_unique<Expression>(std::move(expr))) {}

BinaryExpression::BinaryExpression(
    Expression &&lhs, BinaryOperator op, Expression &&rhs
)
    : lhs(std::make_unique<Expression>(std::move(lhs))), op(op),
      rhs(std::make_unique<Expression>(std::move(rhs))) {}

} // namespace l3::ast
