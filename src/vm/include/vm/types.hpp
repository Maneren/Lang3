#pragma once

#include "ast/nodes/function.hpp"
#include "ast/nodes/identifier.hpp"
#include "utils/cow.h"
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

class Scope;

struct Nil {};

using PrimitiveType = std::variant<bool, long long, double, std::string>;
class Primitive : public PrimitiveType {
public:
  explicit Primitive(bool value) : PrimitiveType{value} {}
  explicit Primitive(long long value) : PrimitiveType{value} {}
  explicit Primitive(double value) : PrimitiveType{value} {}
  explicit Primitive(const std::string &value) : PrimitiveType{value} {}
  explicit Primitive(std::string &&value) : PrimitiveType{std::move(value)} {}

  auto visit(auto &&...visitor) const {
    return match::match(*this, std::forward<decltype(visitor)>(visitor)...);
  }

  [[nodiscard]] const PrimitiveType &get() const { return *this; }
  [[nodiscard]] PrimitiveType &get() { return *this; }
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
  Value() : inner{Nil{}} {}
  Value(Nil /*unused*/) : inner{Nil{}} {}
  Value(Primitive &&primitive) : inner{std::move(primitive)} {}
  Value(std::shared_ptr<Function> function) : inner{std::move(function)} {}

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

private:
  template <typename Op>
  [[nodiscard]] Value
  binary_op(const Value &other, Op op, const std::string &op_name) const {
    return match::match(
        std::make_tuple(inner, other.inner),
        [&op](const Primitive &lhs, const Primitive &rhs) -> Value {
          return op(lhs, rhs);
        },
        [](const Nil & /*lhs*/, const Nil & /*rhs*/) -> Value { return Nil{}; },
        [&op_name](const auto & /*lhs*/, const auto & /*rhs*/) -> Value {
          throw RuntimeError(
              std::format("cannot {} function and value", op_name)
          );
        }
    );
  }

  std::variant<Nil, Primitive, std::shared_ptr<Function>> inner;
};

using CowValue = utils::Cow<Value>;

class Scope {
public:
  Scope() = default;

  Value &declare_variable(const ast::Identifier &id);

  std::optional<std::reference_wrapper<Value>>
  get_variable(const ast::Identifier &id) const;

private:
  std::unordered_map<ast::Identifier, std::unique_ptr<Value>> variables;
};

class Function {
  Scope capture_scope;
  ast::FunctionBody body;
};

} // namespace l3::vm

template <> struct std::formatter<l3::vm::Nil> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }
  static constexpr auto
  format(const l3::vm::Nil & /*unused*/, std::format_context &ctx) {
    return std::format_to(ctx.out(), "nil");
  }
};

template <> struct std::formatter<l3::vm::Primitive> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }
  static constexpr auto
  format(const l3::vm::Primitive &primitive, std::format_context &ctx) {
    return match::match(
        primitive,
        [&ctx](const bool &value) {
          return std::format_to(ctx.out(), "{}", value);
        },
        [&ctx](const long long &value) {
          return std::format_to(ctx.out(), "{}", value);
        },
        [&ctx](const double &value) {
          return std::format_to(ctx.out(), "{}", value);
        },
        [&ctx](const std::string &value) {
          return std::format_to(ctx.out(), "\"{}\"", value);
        }
    );
  }
};

template <> struct std::formatter<l3::vm::Value> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }
  static constexpr auto
  format(const l3::vm::Value &value, std::format_context &ctx) {
    return value.visit(
        [&ctx](const l3::vm::Nil &value) {
          return std::format_to(ctx.out(), "{}", value);
        },
        [&ctx](const l3::vm::Primitive &primitive) {
          return std::format_to(ctx.out(), "{}", primitive);
        },
        [/*&ctx*/](const auto & /*function*/) -> decltype(ctx.out()) {
          throw std::runtime_error("not implemented");
          // return std::format_to(ctx.out(), "{}", function);
        }
    );
  }
};
