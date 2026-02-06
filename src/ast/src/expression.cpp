module;

#include <deque>
#include <memory>
#include <utility>

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
  comparisons.emplace_back(op, std::move(right));
}
bool Comparison::add_comparison(ComparisonOperator op, Expression &&right) {
  if (type != get_type(op)) {
    return false;
  }
  comparisons.emplace_back(op, std::move(right));
  return true;
}

Expression::Expression() = default;
Expression::Expression(AnonymousFunction &&function)
    : inner(std::move(function)) {}
Expression::Expression(BinaryExpression &&expression)
    : inner(std::move(expression)) {}
Expression::Expression(Comparison &&comparison)
    : inner(std::move(comparison)) {}
Expression::Expression(FunctionCall &&call) : inner(std::move(call)) {}
Expression::Expression(IfExpression &&clause) : inner(std::move(clause)) {}
Expression::Expression(IndexExpression &&index) : inner(std::move(index)) {}
Expression::Expression(Literal &&literal) : inner(std::move(literal)) {}
Expression::Expression(LogicalExpression &&expression)
    : inner(std::move(expression)) {}
Expression::Expression(UnaryExpression &&expression)
    : inner(std::move(expression)) {}
Expression::Expression(Variable &&variable) : inner(std::move(variable)) {}

ExpressionList::ExpressionList() = default;
ExpressionList::ExpressionList(ExpressionList &&) noexcept = default;
ExpressionList &ExpressionList::operator=(ExpressionList &&) noexcept = default;

ExpressionList::ExpressionList(Expression &&expression) {
  emplace_front(std::move(expression));
};
ExpressionList &ExpressionList::with_expression(Expression &&expression) {
  emplace_front(std::move(expression));
  return *this;
}

ExpressionList::~ExpressionList() = default;

} // namespace l3::ast
