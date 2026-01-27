module;

#include <utility>

module l3.ast;

import :expression;

char decode_escape(char c) {
  switch (c) {
  case '\\':
    return '\\';
  case 'n':
    return '\n';
  case 't':
    return '\t';
  default:
    return c;
  }
}

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
