#pragma once

#include "utils/accessor.h"
#include "vm/storage.hpp"
#include <ast/ast.hpp>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <utils/cow.h>
#include <utils/debug.h>
#include <utils/match.h>
#include <utils/types.h>
#include <variant>

template <> struct std::hash<l3::ast::Identifier> {
  std::size_t operator()(const l3::ast::Identifier &id) const {
    return std::hash<std::string>{}(id.get_name());
  }
};

namespace l3::vm {

struct Nil {};

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

  auto visit(auto &&...visitor) const {
    return match::match(inner, std::forward<decltype(visitor)>(visitor)...);
  }

  DEFINE_ACCESSOR(inner, decltype(inner), inner);

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

class Function;

using NewValue = std::variant<RefValue, Value>;

struct Slice {
  std::optional<std::int64_t> start, end;
};

class Value {

public:
  using function_type = std::shared_ptr<Function>;
  using vector_type = std::vector<RefValue>;
  Value();

  Value(const Value &) = delete;
  Value(Value &&) = default;
  Value &operator=(const Value &) = delete;
  Value &operator=(Value &&) = default;
  ~Value() = default;

  Value(Nil /*unused*/);
  Value(Primitive &&primitive);
  Value(Function &&function);
  Value(function_type function);
  Value(vector_type &&vector);

  [[nodiscard]] Value add(const Value &other) const;
  [[nodiscard]] Value sub(const Value &other) const;
  [[nodiscard]] Value mul(const Value &other) const;
  [[nodiscard]] Value div(const Value &other) const;
  [[nodiscard]] Value mod(const Value &other) const;
  [[nodiscard]] Value and_op(const Value &other) const;
  [[nodiscard]] Value or_op(const Value &other) const;
  [[nodiscard]] Value equal(const Value &other) const;
  [[nodiscard]] Value not_equal(const Value &other) const;
  [[nodiscard]] Value greater(const Value &other) const;
  [[nodiscard]] Value greater_equal(const Value &other) const;
  [[nodiscard]] Value less(const Value &other) const;
  [[nodiscard]] Value less_equal(const Value &other) const;

  [[nodiscard]] Value not_op() const;
  [[nodiscard]] Value negative() const;

  auto visit(auto &&...visitor) const {
    return match::match(inner, std::forward<decltype(visitor)>(visitor)...);
  }

  auto visit(auto &&...visitor) {
    return match::match(inner, std::forward<decltype(visitor)>(visitor)...);
  }

  [[nodiscard]] bool is_nil() const;
  [[nodiscard]] bool is_function() const;
  [[nodiscard]] bool is_primitive() const;
  [[nodiscard]] bool is_vector() const;

  [[nodiscard]] utils::optional_cref<Primitive> as_primitive() const;
  [[nodiscard]] utils::optional_cref<function_type> as_function() const;
  [[nodiscard]] utils::optional_cref<vector_type> as_vector() const;
  [[nodiscard]] utils::optional_ref<vector_type> as_mut_vector();

  [[nodiscard]] bool is_truthy() const;

  [[nodiscard]] NewValue index(const Value &index) const;
  [[nodiscard]] NewValue index(size_t index) const;

  [[nodiscard]] Value slice(Slice slice) const;

  [[nodiscard]] std::string_view type_name() const;

private:
  [[nodiscard]] Value binary_op(
      const Value &other,
      const std::function<Value(const Primitive &, const Primitive &)> &op_fn,
      const std::string &op_name
  ) const;

  std::variant<Nil, Primitive, function_type, vector_type> inner;
};

} // namespace l3::vm
