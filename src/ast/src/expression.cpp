module;

#include <memory>

module l3.ast;

namespace l3::ast {

UnaryExpression::UnaryExpression(UnaryOperator op, Expression &&expression)
    : op(op), expression(std::make_unique<Expression>(std::move(expression))) {}

BinaryExpression::BinaryExpression(
    Expression &&lhs, BinaryOperator op, Expression &&rhs
)
    : lhs(std::make_unique<Expression>(std::move(lhs))), op(op),
      rhs(std::make_unique<Expression>(std::move(rhs))) {}

LogicalExpression::LogicalExpression(
    Expression &&lhs, LogicalOperator op, Expression &&rhs
)
    : lhs(std::make_unique<Expression>(std::move(lhs))), op(op),
      rhs(std::make_unique<Expression>(std::move(rhs))) {}

} // namespace l3::ast
