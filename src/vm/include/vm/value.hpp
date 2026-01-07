#pragma once

#include "ast/printing.hpp"
#include "utils/cow.h"
#include "utils/format.h"
#include "utils/match.h"
#include "vm/error.hpp"
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <variant>

template <> struct std::hash<l3::ast::Identifier> {
  size_t operator()(const l3::ast::Identifier &id) const {
    return std::hash<std::string>{}(id.name());
  }
};

namespace l3::vm {

struct Nil {};

using PrimitiveType =
    std::variant<bool, long long, double, std::shared_ptr<std::string>>;
class Primitive : public PrimitiveType {
public:
  using string_type = std::shared_ptr<std::string>;
  explicit Primitive(bool value) : PrimitiveType{value} {}
  explicit Primitive(long long value) : PrimitiveType{value} {}
  explicit Primitive(double value) : PrimitiveType{value} {}
  explicit Primitive(const std::string &value)
      : PrimitiveType{std::make_shared<std::string>(value)} {}
  explicit Primitive(std::string &&value)
      : PrimitiveType{std::make_shared<std::string>(std::move(value))} {}

  auto visit(auto &&...visitor) const {
    return match::match(*this, std::forward<decltype(visitor)>(visitor)...);
  }

  [[nodiscard]] const PrimitiveType &get() const { return *this; }
  [[nodiscard]] PrimitiveType &get() { return *this; }

  [[nodiscard]] bool as_bool() const;
};

Primitive operator+(const Primitive &lhs, const Primitive &rhs);
Primitive operator-(const Primitive &lhs, const Primitive &rhs);
Primitive operator*(const Primitive &lhs, const Primitive &rhs);
Primitive operator/(const Primitive &lhs, const Primitive &rhs);
Primitive operator%(const Primitive &lhs, const Primitive &rhs);
Primitive operator&&(const Primitive &lhs, const Primitive &rhs);
Primitive operator||(const Primitive &lhs, const Primitive &rhs);

class Function;

class Value {

public:
  using function_type = std::shared_ptr<Function>;
  Value() : inner{Nil{}} {}

  Value(const Value &) = default;
  Value(Value &&) = default;
  Value &operator=(const Value &) = default;
  Value &operator=(Value &&) = default;
  ~Value() = default;

  Value(Nil /*unused*/) : inner{Nil{}} {}
  Value(Primitive &&primitive) : inner{std::move(primitive)} {}
  Value(Function &&function);
  Value(function_type function) : inner{std::move(function)} {}

  [[nodiscard]] Value add(const Value &other) const;
  [[nodiscard]] Value sub(const Value &other) const;
  [[nodiscard]] Value mul(const Value &other) const;
  [[nodiscard]] Value div(const Value &other) const;
  [[nodiscard]] Value mod(const Value &other) const;
  [[nodiscard]] Value and_op(const Value &other) const;
  [[nodiscard]] Value or_op(const Value &other) const;

  auto visit(auto &&...visitor) const {
    return match::match(inner, std::forward<decltype(visitor)>(visitor)...);
  }

  [[nodiscard]] bool is_nil() const {
    return std::holds_alternative<Nil>(inner);
  }
  [[nodiscard]] bool is_function() const {
    return std::holds_alternative<function_type>(inner);
  }
  [[nodiscard]] bool is_primitive() const {
    return std::holds_alternative<Primitive>(inner);
  }

  [[nodiscard]] bool as_bool() const;

private:
  template <typename Op>
  [[nodiscard]] Value
  binary_op(const Value &other, Op op, const std::string &op_name) const {
    return match::match(
        std::make_tuple(inner, other.inner),
        [&op](const Primitive &lhs, const Primitive &rhs) -> Value {
          return op(lhs, rhs);
        },
        [](const Nil & /*lhs*/, const Nil & /*rhs*/) -> Value {
          throw RuntimeError("cannot perform operation on nil values");
        },
        [&op_name](const auto & /*lhs*/, const auto & /*rhs*/) -> Value {
          throw RuntimeError(
              "cannot {} function and value, did you mean to call the "
              "function?",
              op_name
          );
        }
    );
  }

  std::variant<Nil, Primitive, function_type> inner;
};

using CowValue = utils::Cow<Value>;

} // namespace l3::vm
