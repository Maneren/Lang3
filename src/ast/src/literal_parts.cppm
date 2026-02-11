export module l3.ast:literal_parts;

import std;
import l3.location;

export namespace l3::ast {

struct Nil {
  DEFINE_LOCATION_FIELD()

  constexpr Nil(location::Location location = {})
      : location_(std::move(location)) {}
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
  ); // NOLINT

  DEFINE_ACCESSOR_X(value);
};

class String {
  std::string value;

  DEFINE_LOCATION_FIELD()

public:
  String(const std::string &literal, location::Location location = {});

  DEFINE_ACCESSOR_X(value);
};

} // namespace l3::ast
