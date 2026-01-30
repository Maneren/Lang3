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

} // namespace l3::ast
