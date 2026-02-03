module;

#include <memory>

module l3.ast;

import :expression;

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

Comparison::Comparison(
    Expression &&left, ComparisonOperator op, Expression &&right
)
    : start(std::make_unique<Expression>(std::move(left))), type(get_type(op)) {
  comparisons.emplace_back(op, std::make_unique<Expression>(std::move(right)));
}
bool Comparison::add_comparison(ComparisonOperator op, Expression &&right) {
  if (type != get_type(op)) {
    return false;
  }
  comparisons.emplace_back(op, std::make_unique<Expression>(std::move(right)));
  return true;
}

} // namespace l3::ast
