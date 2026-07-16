export module l3.ast:literal;

import std;

import utils;

import l3.location;

export import :array;

export namespace l3::ast {

struct Nil {
  DEFINE_LOCATION_FIELD()

  constexpr Nil(location::Location location = {}) : location_(location) {}
};

class Boolean {
  bool value;

  DEFINE_LOCATION_FIELD()

public:
  Boolean(bool value, location::Location location = {});

  DEFINE_ACCESSOR_X(value);
};

class Number {
  std::int64_t value;

  DEFINE_LOCATION_FIELD()

public:
  Number(std::int64_t value, location::Location location = {});

  DEFINE_ACCESSOR_X(value);
};

class Float {
  double value;

  DEFINE_LOCATION_FIELD()

public:
  Float(std::int64_t integral, location::Location location = {});
  Float(
      std::int64_t integral,
      std::int64_t fractional,
      location::Location location = {}
  );

  DEFINE_ACCESSOR_X(value);
};

class String {
  std::string value;

  DEFINE_LOCATION_FIELD()

public:
  String(std::string literal, location::Location location = {});

  DEFINE_ACCESSOR_X(value);
};

} // namespace l3::ast

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
