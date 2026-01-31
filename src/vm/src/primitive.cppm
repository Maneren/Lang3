module;

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utils/accessor.h>
#include <utils/visit.h>
#include <variant>

export module l3.vm:primitive;

import utils;

export namespace l3::vm {

class Primitive {
  std::variant<bool, std::int64_t, double, std::string> inner;

public:
  using string_type = std::string;
  explicit Primitive(bool value);
  explicit Primitive(std::int64_t value);
  explicit Primitive(double value);
  explicit Primitive(const std::string &value);
  explicit Primitive(std::string &&value);

  [[nodiscard]] bool is_bool() const;
  [[nodiscard]] bool is_integer() const;
  [[nodiscard]] bool is_double() const;
  [[nodiscard]] bool is_string() const;

  [[nodiscard]] std::optional<bool> as_bool() const;
  [[nodiscard]] std::optional<std::int64_t> as_integer() const;
  [[nodiscard]] std::optional<double> as_double() const;
  [[nodiscard]] utils::optional_cref<string_type> as_string() const;

  VISIT(inner);

  DEFINE_ACCESSOR_X(inner);

  [[nodiscard]] bool is_truthy() const;

  [[nodiscard]] std::string_view type_name() const;
};

Primitive operator+(const Primitive &lhs, const Primitive &rhs);
Primitive operator-(const Primitive &lhs, const Primitive &rhs);
Primitive operator*(const Primitive &lhs, const Primitive &rhs);
Primitive operator/(const Primitive &lhs, const Primitive &rhs);
Primitive operator%(const Primitive &lhs, const Primitive &rhs);
Primitive operator&&(const Primitive &lhs, const Primitive &rhs);
Primitive operator||(const Primitive &lhs, const Primitive &rhs);
Primitive operator==(const Primitive &lhs, const Primitive &rhs);
Primitive operator!=(const Primitive &lhs, const Primitive &rhs);
Primitive operator>(const Primitive &lhs, const Primitive &rhs);
Primitive operator>=(const Primitive &lhs, const Primitive &rhs);
Primitive operator<(const Primitive &lhs, const Primitive &rhs);
Primitive operator<=(const Primitive &lhs, const Primitive &rhs);
Primitive operator!(const Primitive &value);
Primitive operator-(const Primitive &value);
Primitive operator+(const Primitive &value);

} // namespace l3::vm
