module;

#include <iterator>
#include <utility>

module l3.ast;

import :expression;

namespace l3::ast {

ExpressionList::ExpressionList() = default;
ExpressionList::ExpressionList(ExpressionList &&) noexcept = default;
ExpressionList &ExpressionList::operator=(ExpressionList &&) noexcept = default;

ExpressionList::ExpressionList(Expression &&expr) {
  emplace_front(std::move(expr));
};
ExpressionList &ExpressionList::with_expression(Expression &&expr) {
  emplace_front(std::move(expr));
  return *this;
}

ExpressionList::~ExpressionList() = default;

} // namespace l3::ast
