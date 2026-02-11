export module l3.ast:literal;

import utils;
export import :literal_parts;
export import :array;
import l3.location;

char decode_escape(char c);

export namespace l3::ast {

class Literal {
  std::variant<Nil, Boolean, Number, Float, String, Array> inner;

public:
  Literal();
  Literal(Nil nil);
  Literal(Boolean boolean);
  Literal(Number num);
  Literal(Float num);
  Literal(String &&string);
  Literal(Array &&array);

  VISIT(inner)

  [[nodiscard]] const location::Location &get_location() const;
};

} // namespace l3::ast
