#pragma once

#include "vm/ref_value.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <utils/accessor.h>
#include <utils/visit.h>
#include <variant>

import utils;

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

class Function;

using NewValue = std::variant<RefValue, Value>;

struct Slice {
  std::optional<std::int64_t> start, end;
};

class Value {

public:
  using function_type = std::shared_ptr<Function>;
  using vector_type = std::vector<RefValue>;
  using vector_ptr_type = std::shared_ptr<vector_type>;

  Value();

  Value(const Value &) = delete;
  Value(Value &&) = default;
  Value &operator=(const Value &) = delete;
  Value &operator=(Value &&) = default;
  ~Value() = default;

  Value(Nil /*unused*/);
  Value(Primitive &&primitive);
  Value(Function &&function);
  Value(function_type &&function);
  Value(vector_ptr_type &&vector);
  Value(vector_type &&vector);

  [[nodiscard]] Value clone() const;

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

  VISIT(inner);

  [[nodiscard]] bool is_nil() const;
  [[nodiscard]] bool is_function() const;
  [[nodiscard]] bool is_primitive() const;
  [[nodiscard]] bool is_vector() const;

  [[nodiscard]] utils::optional_cref<Primitive> as_primitive() const;
  [[nodiscard]] utils::optional_cref<function_type> as_function() const;
  [[nodiscard]] utils::optional_cref<vector_type> as_vector() const;
  [[nodiscard]] utils::optional_ref<vector_type> as_mut_vector();

  [[nodiscard]] bool is_truthy() const;
  [[nodiscard]] bool is_falsy() const { return !is_truthy(); }

  [[nodiscard]] NewValue index(const Value &index) const;
  [[nodiscard]] NewValue index(size_t index) const;

  [[nodiscard]] RefValue &index_mut(const Value &index);
  [[nodiscard]] RefValue &index_mut(size_t index);

  [[nodiscard]] Value slice(Slice slice) const;

  [[nodiscard]] std::string_view type_name() const;

private:
  [[nodiscard]] Value binary_op(
      const Value &other,
      const std::function<Value(const Primitive &, const Primitive &)> &op_fn,
      const std::string &op_name
  ) const;

  std::variant<Nil, Primitive, function_type, vector_ptr_type> inner;
};

} // namespace l3::vm
