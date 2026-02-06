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

Boolean::Boolean(bool value) : value(value) {}

Number::Number(std::int64_t value) : value(value) {}

Float::Float(std::int64_t integral) : value(static_cast<double>(integral)) {}
Float::Float(std::int64_t integral, std::int64_t fractional) // NOLINT
    : value(static_cast<double>(integral)) {

  auto frac = static_cast<double>(fractional);

  while (frac >= 1.0) {
    frac /= 10.0; // NOLINT
  }
  value += frac;
}

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

Literal::Literal() = default;
Literal::Literal(Nil nil) : inner(nil) {}
Literal::Literal(Boolean boolean) : inner(boolean) {}
Literal::Literal(Number num) : inner(num) {}
Literal::Literal(Float num) : inner(num) {}
Literal::Literal(String &&string) : inner(std::move(string)) {}
Literal::Literal(Array &&array) : inner(std::move(array)) {}

} // namespace l3::ast
