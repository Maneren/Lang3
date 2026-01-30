module;

#include <deque>
#include <utility>

module l3.ast;

namespace l3::ast {

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
