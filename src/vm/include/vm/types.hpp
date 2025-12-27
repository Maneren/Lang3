#pragma once

#include "ast/nodes/function.hpp"
#include "ast/nodes/identifier.hpp"
#include "utils/cow.h"
#include "utils/format.h"
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
  using function_type = std::shared_ptr<Function>;
  Value() : inner{Nil{}} {}
  Value(Nil /*unused*/) : inner{Nil{}} {}
  Value(Primitive &&primitive) : inner{std::move(primitive)} {}
  Value(Function &&function)
      : inner{std::make_shared<Function>(std::move(function))} {}
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

  std::variant<Nil, Primitive, function_type> inner;
};

using CowValue = utils::Cow<Value>;

class L3Function {
public:
  L3Function(Scope &&capture_scope, ast::FunctionBody body);

  CowValue operator()(std::span<const CowValue> /*args*/) const {
    throw std::runtime_error("not implemented");
  }

private:
  std::unique_ptr<Scope> capture_scope;
  ast::FunctionBody body;
};

class BuiltinFunction {
public:
  BuiltinFunction(std::function<CowValue(std::span<const CowValue>)> body)
      : body{std::move(body)} {}

  CowValue operator()(std::span<const CowValue> args) const {
    return body(args);
  }

private:
  std::function<CowValue(std::span<const CowValue>)> body;
};

class Scope {
public:
  Scope() = default;

  static Scope global();

  Value &declare_variable(const ast::Identifier &id);

  std::optional<std::reference_wrapper<Value>>
  get_variable(const ast::Identifier &id) const;

private:
  std::unordered_map<ast::Identifier, std::unique_ptr<Value>> variables;
};

class Function {
public:
  Function(L3Function &&function) : inner{std::move(function)} {}
  Function(BuiltinFunction &&function) : inner{std::move(function)} {}

  CowValue operator()(std::span<const CowValue> args) const;

private:
  std::variant<L3Function, BuiltinFunction> inner;
};

} // namespace l3::vm

template <>
struct std::formatter<l3::vm::Nil> : utils::static_formatter<l3::vm::Nil> {
  static constexpr auto
  format(const auto & /*unused*/, std::format_context &ctx) {
    return std::format_to(ctx.out(), "nil");
  }
};

template <>
struct std::formatter<l3::vm::Primitive>
    : utils::static_formatter<l3::vm::Primitive> {
  static constexpr auto
  format(const auto &primitive, std::format_context &ctx) {
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

template <>
struct std::formatter<l3::vm::Value> : utils::static_formatter<l3::vm::Value> {
  static constexpr auto format(const auto &value, std::format_context &ctx) {
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
