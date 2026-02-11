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

Boolean::Boolean(bool value, location::Location location)
    : value(value), location_(std::move(location)) {}

Number::Number(std::int64_t value, location::Location location)
    : value(value), location_(std::move(location)) {}

Float::Float(std::int64_t integral, location::Location location)
    : value(static_cast<double>(integral)), location_(std::move(location)) {}
Float::Float(
    std::int64_t integral,
    std::int64_t fractional,
    location::Location location
) // NOLINT
    : value(static_cast<double>(integral)), location_(std::move(location)) {

  auto frac = static_cast<double>(fractional);

  while (frac >= 1.0) {
    frac /= 10.0; // NOLINT
  }
  value += frac;
}

String::String(const std::string &literal, location::Location location)
    : location_(std::move(location)) {
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

const location::Location &Literal::get_location() const {
  return std::visit(
      [](const auto &node) -> const location::Location & {
        return node.get_location();
      },
      inner
  );
}

} // namespace l3::ast
