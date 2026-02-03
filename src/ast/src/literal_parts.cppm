module;

#include <cstdint>
#include <string>
#include <utils/accessor.h>

export module l3.ast:literal_parts;

export namespace l3::ast {

struct Nil {};

class Boolean {
  bool value;

public:
  Boolean(bool value);

  DEFINE_ACCESSOR_X(value);
};

class Number {
  std::int64_t value;

public:
  Number(std::int64_t value);

  DEFINE_ACCESSOR_X(value);
};

class Float {
  double value;

public:
  Float(std::int64_t integral);
  Float(std::int64_t integral, std::int64_t fractional); // NOLINT

  DEFINE_ACCESSOR_X(value);
};

class String {
  std::string value;

public:
  String(const std::string &literal);

  DEFINE_ACCESSOR_X(value);
};

} // namespace l3::ast
