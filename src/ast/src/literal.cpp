module;

#include <ranges>
#include <string>
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

String::String(const std::string &literal) {
  using namespace std::ranges;

  value.reserve(literal.size());

  for (auto &&[index, segment] :
       literal | views::split('\\') | views::enumerate) {
    if (index == 0) {
      // First segment has no preceding escape character
      value.append_range(segment);
    } else if (!segment.empty()) {
      value += decode_escape(segment.front());
      value.append_range(segment | views::drop(1));
    } else {
      // Do nothing if the segment is empty
    }
  }
}

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
