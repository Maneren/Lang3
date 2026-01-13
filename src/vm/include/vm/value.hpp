#pragma once

#include "utils/debug.h"
#include "vm/storage.hpp"
#include <ast/nodes/identifier.hpp>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <utils/cow.h>
#include <utils/match.h>
#include <variant>

template <> struct std::hash<l3::ast::Identifier> {
  std::size_t operator()(const l3::ast::Identifier &id) const {
    return std::hash<std::string>{}(id.name());
  }
};

namespace l3::vm {

struct Nil {};

using PrimitiveType = std::variant<bool, long long, double, std::string>;
class Primitive : public PrimitiveType {
public:
  using string_type = std::string;
  explicit Primitive(bool value);
  explicit Primitive(long long value);
  explicit Primitive(double value);
  explicit Primitive(const std::string &value);
  explicit Primitive(std::string &&value);

  auto visit(auto &&...visitor) const {
    return match::match(*this, std::forward<decltype(visitor)>(visitor)...);
  }

  [[nodiscard]] const PrimitiveType &get() const;
  [[nodiscard]] PrimitiveType &get();

  [[nodiscard]] bool as_bool() const;
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

class Function;

class Value {

public:
  using function_type = std::shared_ptr<Function>;
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

  auto visit(auto &&...visitor) const {
    return match::match(inner, std::forward<decltype(visitor)>(visitor)...);
  }

  [[nodiscard]] bool is_nil() const;
  [[nodiscard]] bool is_function() const;
  [[nodiscard]] bool is_primitive() const;

  [[nodiscard]] bool as_bool() const;

private:
  [[nodiscard]] Value binary_op(
      const Value &other,
      const std::function<Value(const Primitive &, const Primitive &)> &op_fn,
      const std::string &op_name
  ) const;

  std::variant<Nil, Primitive, function_type> inner;
};

struct RefValue {
  explicit RefValue(GCValue &gc_value) : gc_value{gc_value} {}

  [[nodiscard]] const Value &get() const { return gc_value.get().get(); }
  [[nodiscard]] Value &get() { return gc_value.get().get(); }

  [[nodiscard]] const GCValue &get_gc() const { return gc_value.get(); }
  [[nodiscard]] GCValue &get_gc() { return gc_value.get(); }

  [[nodiscard]] Value &operator*() { return get(); }
  [[nodiscard]] const Value &operator*() const { return get(); }

  Value *operator->() { return &get(); }
  const Value *operator->() const { return &get(); }

private:
  std::reference_wrapper<GCValue> gc_value;
};

} // namespace l3::vm
