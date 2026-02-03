module;

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>

module l3.vm;

import utils;
import :error;

namespace l3::vm {

namespace {

template <bool backup = true>
auto handle_bin_op(
    std::string_view name,
    const Primitive &lhs,
    const Primitive &rhs,
    auto &&...handlers
) {
  using Result = std::invoke_result_t<
      match::Overloaded<std::decay_t<decltype(handlers)>...>,
      const std::int64_t &,
      const std::int64_t &>;
  return match::match(
      std::forward_as_tuple(lhs.get_inner(), rhs.get_inner()),
      handlers...,
      [name](const auto &lhs, const auto &rhs) -> Result
        requires(backup)
      { throw UnsupportedOperation(name, lhs, rhs); }
      );
}

template <typename Result>
auto handle_un_op(
    std::string_view name, const Primitive &value, auto &&...handlers
) {
  return value.visit(handlers..., [name](const auto &value) -> Result {
    throw UnsupportedOperation(name, value);
  });
}

} // namespace

Primitive operator+(const Primitive &lhs, const Primitive &rhs) {
  return handle_bin_op(
      "addition",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires(!std::is_same_v<T, bool>)
      { return Primitive{lhs + rhs}; }
      );
}

Primitive operator-(const Primitive &lhs, const Primitive &rhs) {
  return handle_bin_op(
      "subtraction",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires requires(T lhs, T rhs) { lhs - rhs; } &&
                 (!std::is_same_v<T, bool>)
      { return Primitive{lhs - rhs}; }
      );
}

Primitive operator*(const Primitive &lhs, const Primitive &rhs) {
  return handle_bin_op(
      "multiplication",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires requires(T lhs, T rhs) { lhs * rhs; } &&
                 (!std::is_same_v<T, bool>)
      { return Primitive{lhs * rhs}; }
      );
}

Primitive operator/(const Primitive &lhs, const Primitive &rhs) {
  return handle_bin_op(
      "division",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires requires(T lhs, T rhs) { lhs / rhs; } &&
                 (!std::is_same_v<T, bool>)
      {
        if (rhs == static_cast<T>(0)) {
          throw UnsupportedOperation("division by zero");
        }
        return Primitive{lhs / rhs};
      }
      );
}

Primitive operator%(const Primitive &lhs, const Primitive &rhs) {
  return handle_bin_op(
      "modulo",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires requires(T lhs, T rhs) { lhs % rhs; } &&
                 (!std::is_same_v<T, bool>)
      { return Primitive{lhs % rhs}; }
      );
}

Primitive operator!(const Primitive &value) {
  return Primitive{!value.is_truthy()};
}

Primitive operator-(const Primitive &value) {
  return handle_un_op<Primitive>(
      "negative",
      value,
      []<typename T>(const T &value) -> Primitive
        requires requires(T value) { -value; } && (!std::is_same_v<T, bool>)
      { return Primitive{-value}; }
      );
}

Primitive operator+(const Primitive &value) {
  return handle_un_op<Primitive>(
      "positive",
      value,
      []<typename T>(const T &value) -> Primitive
        requires requires(T value) { +value; } && (!std::is_same_v<T, bool>)
      { return Primitive{+value}; }
      );
}

std::partial_ordering operator<=>(const Primitive &lhs, const Primitive &rhs) {
  return handle_bin_op<false>(
      "comparison",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> std::partial_ordering
        requires requires(T lhs, T rhs) { lhs <=> rhs; }
      { return lhs <=> rhs; },
      [](const auto &, const auto &) {
        return std::partial_ordering::unordered;
      }
      );
}

bool Primitive::is_truthy() const {
  return visit(
      [](const bool &value) { return value; },
      [](const std::int64_t &value) { return value != 0; },
      [](const double & /*value*/) -> bool {
        throw RuntimeError("cannot convert a floating point number to bool");
      }
  );
}

std::optional<double> Primitive::as_double() const {
  return visit(
      [](const double &value) -> std::optional<double> { return value; },
      [](const auto &) -> std::optional<double> { return std::nullopt; }
  );
}
std::optional<std::int64_t> Primitive::as_integer() const {
  return visit(
      [](const std::int64_t &value) -> std::optional<std::int64_t> {
        return value;
      },
      [](const auto &) -> std::optional<std::int64_t> { return std::nullopt; }
  );
}
std::optional<bool> Primitive::as_bool() const {
  return visit(
      [](const bool &value) -> std::optional<bool> { return value; },
      [](const auto &) -> std::optional<bool> { return std::nullopt; }
  );
}

Primitive::Primitive(bool value) : inner{value} {}
Primitive::Primitive(std::int64_t value) : inner{value} {}
Primitive::Primitive(double value) : inner{value} {}

bool Primitive::is_bool() const { return std::holds_alternative<bool>(inner); }
bool Primitive::is_integer() const {
  return std::holds_alternative<std::int64_t>(inner);
}
bool Primitive::is_double() const {
  return std::holds_alternative<double>(inner);
}

std::string_view Primitive::type_name() const {
  return visit(
      [](const bool &) -> std::string_view { return "bool"; },
      [](const std::int64_t &) -> std::string_view { return "int"; },
      [](const double &) -> std::string_view { return "double"; },
      [](const string_type &) -> std::string_view { return "string"; }
  );
}

} // namespace l3::vm
