#include "vm/value.hpp"
#include "vm/error.hpp"
#include "vm/function.hpp"
#include <memory>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>

namespace l3::vm {

namespace {

template <typename Result, typename... Handlers>
auto handle_op(
    std::string_view name,
    const Primitive &lhs,
    const Primitive &rhs,
    Handlers... handlers
) {
  return match::match(
      std::forward_as_tuple(lhs.get(), rhs.get()),
      handlers...,
      [name](const auto &lhs, const auto &rhs) -> Result {
        throw UnsupportedOperation(name, lhs, rhs);
      }
  );
}

} // namespace

Primitive operator+(const Primitive &lhs, const Primitive &rhs) {
  return handle_op<Primitive>(
      "addition",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires(!std::is_same_v<T, bool>)
      { return Primitive{lhs + rhs}; }
      );
}

Primitive operator-(const Primitive &lhs, const Primitive &rhs) {
  return handle_op<Primitive>(
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
  return handle_op<Primitive>(
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
  return handle_op<Primitive>(
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
  return handle_op<Primitive>(
      "modulo",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires requires(T lhs, T rhs) { lhs % rhs; } &&
                 (!std::is_same_v<T, bool>)
      { return Primitive{lhs % rhs}; }
      );
}

Primitive operator&&(const Primitive &lhs, const Primitive &rhs) {
  return Primitive{lhs.as_bool() && rhs.as_bool()};
}

Primitive operator||(const Primitive &lhs, const Primitive &rhs) {
  return Primitive{lhs.as_bool() || rhs.as_bool()};
}

Primitive operator!(const Primitive &value) {
  return Primitive{!value.as_bool()};
}

namespace {
int operator<=>(const Primitive &lhs, const Primitive &rhs) {
  return handle_op<int>(
      "comparison",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> int
        requires(!std::is_same_v<T, bool>)
      {
        auto result = lhs <=> rhs;
        if (result < 0) {
          return -1;
        }
        if (result > 0) {
          return 1;
        }
        return 0;
      }
      );
}
} // namespace

Primitive operator!=(const Primitive &lhs, const Primitive &rhs) {
  return Primitive{(lhs <=> rhs) != 0};
}

Primitive operator==(const Primitive &lhs, const Primitive &rhs) {
  return Primitive{(lhs <=> rhs) == 0};
}

Primitive operator<(const Primitive &lhs, const Primitive &rhs) {
  return Primitive{(lhs <=> rhs) < 0};
}

Primitive operator>(const Primitive &lhs, const Primitive &rhs) {
  return Primitive{(lhs <=> rhs) > 0};
}

Primitive operator<=(const Primitive &lhs, const Primitive &rhs) {
  return Primitive{(lhs <=> rhs) <= 0};
}

Primitive operator>=(const Primitive &lhs, const Primitive &rhs) {
  return Primitive{(lhs <=> rhs) >= 0};
}

Value Value::add(const Value &other) const {
  return binary_op(
      other, [](const auto &lhs, const auto &rhs) { return lhs + rhs; }, "add"
  );
}

Value Value::sub(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs - rhs; },
      "subtract"
  );
}

Value Value::mul(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs * rhs; },
      "multiply"
  );
}

Value Value::div(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs / rhs; },
      "divide"
  );
}

Value Value::mod(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs % rhs; },
      "modulo"
  );
}

Value Value::and_op(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs && rhs; },
      "perform logical and on"
  );
}

Value Value::or_op(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs || rhs; },
      "perform logical or on"
  );
}

Value Value::equal(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs == rhs; },
      "compare"
  );
};

Value Value::not_equal(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs != rhs; },
      "compare"
  );
}

Value Value::greater(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs > rhs; },
      "compare"
  );
}

Value Value::greater_equal(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs >= rhs; },
      "compare"
  );
}

Value Value::less(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs < rhs; },
      "compare"
  );
}

Value Value::less_equal(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs <= rhs; },
      "compare"
  );
}

bool Primitive::as_bool() const {
  return visit(
      [](const bool &value) { return value; },
      [](const long long &value) { return value != 0; },
      [](const string_type &value) { return !value.empty(); },
      [](const double & /*value*/) -> bool {
        throw RuntimeError("cannot convert a floating point number to bool");
      }
  );
}
bool Value::as_bool() const {
  return visit(
      [](const Primitive &primitive) { return primitive.as_bool(); },
      [](const Nil & /*value*/) { return false; },
      [](const function_type & /*value*/) -> bool {
        throw RuntimeError(
            "cannot convert a function to bool, did you mean to call the "
            "function?"
        );
      }
  );
}

Value::Value(Function &&function)
    : inner{std::make_shared<Function>(std::move(function))} {}

Primitive::Primitive(bool value) : PrimitiveType{value} {}
Primitive::Primitive(long long value) : PrimitiveType{value} {}
Primitive::Primitive(double value) : PrimitiveType{value} {}
Primitive::Primitive(const std::string &value) : PrimitiveType{value} {}
Primitive::Primitive(std::string &&value) : PrimitiveType{std::move(value)} {}
const PrimitiveType &Primitive::get() const { return *this; }
PrimitiveType &Primitive::get() { return *this; }
Value::Value() : inner{Nil{}} {}
Value::Value(Nil /*unused*/) : inner{Nil{}} {}
Value::Value(Primitive &&primitive) : inner{std::move(primitive)} {}
Value::Value(function_type function) : inner{std::move(function)} {}
bool Value::is_nil() const { return std::holds_alternative<Nil>(inner); }
bool Value::is_function() const {
  return std::holds_alternative<function_type>(inner);
}
bool Value::is_primitive() const {
  return std::holds_alternative<Primitive>(inner);
}

Value Value::binary_op(
    const Value &other,
    const std::function<Value(const Primitive &, const Primitive &)> &op_fn,
    const std::string &op_name
) const {
  return match::match(
      std::forward_as_tuple(inner, other.inner),
      [&op_fn](const Primitive &lhs, const Primitive &rhs) -> Value {
        return op_fn(lhs, rhs);
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

} // namespace l3::vm
