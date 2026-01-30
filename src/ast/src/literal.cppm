module;

#include <deque>
#include <string>
#include <utils/visit.h>
#include <variant>

export module l3.ast:literal;

export import :literal_parts;
export import :array;

char decode_escape(char c);

export namespace l3::ast {

class Literal {
  std::variant<Nil, Boolean, Number, Float, String, Array> inner;

public:
  Literal() = default;
  Literal(Nil nil) : inner(nil) {}
  Literal(Boolean boolean) : inner(boolean) {}
  Literal(Number num) : inner(num) {}
  Literal(Float num) : inner(num) {}
  Literal(String &&string) : inner(std::move(string)) {}
  Literal(Array &&array) : inner(std::move(array)) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    visit([&out, depth](const auto &node) -> void { node.print(out, depth); });
  }

  VISIT(inner)
};

} // namespace l3::ast
