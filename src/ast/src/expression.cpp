module l3.ast;

import :expression;

namespace l3::ast {

UnaryExpression::UnaryExpression(
    UnaryOperator op, Expression &&expression, location::Location location
)
    : op(op), expression(std::make_unique<Expression>(std::move(expression))),
      location_(std::move(location)) {}

BinaryExpression::BinaryExpression(
    Expression &&lhs,
    BinaryOperator op,
    Expression &&rhs,
    location::Location location
)
    : lhs(std::make_unique<Expression>(std::move(lhs))), op(op),
      rhs(std::make_unique<Expression>(std::move(rhs))),
      location_(std::move(location)) {}

LogicalExpression::LogicalExpression(
    Expression &&lhs,
    LogicalOperator op,
    Expression &&rhs,
    location::Location location
)
    : lhs(std::make_unique<Expression>(std::move(lhs))), op(op),
      rhs(std::make_unique<Expression>(std::move(rhs))),
      location_(std::move(location)) {}

Comparison::Comparison(
    Expression &&left,
    ComparisonOperator op,
    Expression &&right,
    location::Location location
)
    : start(std::make_unique<Expression>(std::move(left))), type(get_type(op)),
      location_(std::move(location)) {
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

const location::Location &Expression::get_location() const {
  return std::visit(
      [](const auto &node) -> const location::Location & {
        return node.get_location();
      },
      inner
  );
}

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
