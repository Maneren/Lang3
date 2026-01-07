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

template <typename... Handlers>
auto handle_op(
    std::string_view name,
    const Primitive &lhs,
    const Primitive &rhs,
    Handlers... handlers
) {
  return match::match(
      std::forward_as_tuple(lhs.get(), rhs.get()),
      handlers...,
      [name](const auto &lhs, const auto &rhs) -> Primitive {
        throw UnsupportedOperation(name, lhs, rhs);
      }
  );
}

} // namespace

Primitive operator+(const Primitive &lhs, const Primitive &rhs) {
  return handle_op(
      "addition",
      lhs,
      rhs,
      [](const bool &lhs, const bool &rhs) -> Primitive {
        return Primitive{lhs || rhs};
      },
      []<typename T, typename U>(const T &lhs, const U &rhs) -> Primitive
        requires std::is_same_v<T, U> && requires(T lhs, U rhs) { lhs + rhs; }
      { return Primitive{lhs + rhs}; }
      );
}

Primitive operator-(const Primitive &lhs, const Primitive &rhs) {
  return handle_op(
      "subtraction",
      lhs,
      rhs,
      [](const bool &lhs, const bool &rhs) -> Primitive {
        return Primitive{lhs && !rhs};
      },
      []<typename T, typename U>(const T &lhs, const U &rhs) -> Primitive
        requires std::is_same_v<T, U> && requires(T lhs, U rhs) { lhs - rhs; }
      { return Primitive{lhs - rhs}; }
      );
}

Primitive operator*(const Primitive &lhs, const Primitive &rhs) {
  return handle_op(
      "multiplication",
      lhs,
      rhs,
      [](const bool &lhs, const bool &rhs) -> Primitive {
        return Primitive{lhs && rhs};
      },
      []<typename T, typename U>(const T &lhs, const U &rhs) -> Primitive
        requires std::is_same_v<T, U> && requires(T lhs, U rhs) { lhs * rhs; }
      { return Primitive{lhs * rhs}; }
      );
}

Primitive operator/(const Primitive &lhs, const Primitive &rhs) {
  return handle_op(
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
  return handle_op(
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
  return handle_op(
      "logical conjunction (and)",
      lhs,
      rhs,
      [](const bool &lhs, const bool &rhs) -> Primitive {
        return Primitive{lhs && rhs};
      }
      // ,
      // []<typename T, typename U>(const T &lhs, const U &rhs) -> Primitive
      //   requires std::is_same_v<T, U> && requires(T lhs, U rhs) { lhs &&
      //   rhs; }
      // { return Primitive{lhs && rhs}; }
  );
}

Primitive operator||(const Primitive &lhs, const Primitive &rhs) {
  return handle_op(
      "logical disjunction (or)",
      lhs,
      rhs,
      [](const bool &lhs, const bool &rhs) -> Primitive {
        return Primitive{lhs || rhs};
      }
      // ,
      // []<typename T, typename U>(const T &lhs, const U &rhs) -> Primitive
      //   requires std::is_same_v<T, U> && requires(T lhs, U rhs) { lhs || rhs;
      //   }
      // { return Primitive{lhs || rhs}; }
  );
}

Primitive operator!(const Primitive &value) {
  return Primitive{!value.as_bool()};
}

Primitive operator==(const Primitive &lhs, const Primitive &rhs) {
  return handle_op(
      "equality",
      lhs,
      rhs,
      [](const bool &lhs, const bool &rhs) -> Primitive {
        return Primitive{lhs == rhs};
      },
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires requires(T lhs, T rhs) { lhs == rhs; }
      { return Primitive{lhs == rhs}; }
      );
}

Primitive operator!=(const Primitive &lhs, const Primitive &rhs) {
  return handle_op(
      "inequality",
      lhs,
      rhs,
      [](const bool &lhs, const bool &rhs) -> Primitive {
        return Primitive{lhs != rhs};
      },
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires requires(T lhs, T rhs) { lhs != rhs; }
      { return Primitive{lhs != rhs}; }
      );
}

Primitive operator<(const Primitive &lhs, const Primitive &rhs) {
  return handle_op(
      "less than",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires requires(T lhs, T rhs) { lhs < rhs; } &&
                 (!std::is_same_v<T, bool>)
      { return Primitive{lhs < rhs}; }
      );
}

Primitive operator>(const Primitive &lhs, const Primitive &rhs) {
  return handle_op(
      "greater than",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires requires(T lhs, T rhs) { lhs > rhs; } &&
                 (!std::is_same_v<T, bool>)
      { return Primitive{lhs > rhs}; }
      );
}

Primitive operator<=(const Primitive &lhs, const Primitive &rhs) {
  return handle_op(
      "less than or equal to",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires requires(T lhs, T rhs) { lhs <= rhs; } &&
                 (!std::is_same_v<T, bool>)
      { return Primitive{lhs <= rhs}; }
      );
}

Primitive operator>=(const Primitive &lhs, const Primitive &rhs) {
  return handle_op(
      "greater than or equal to",
      lhs,
      rhs,
      []<typename T>(const T &lhs, const T &rhs) -> Primitive
        requires requires(T lhs, T rhs) { lhs >= rhs; } &&
                 (!std::is_same_v<T, bool>)
      { return Primitive{lhs >= rhs}; }
      );
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
      [](const string_type &value) { return !value->empty(); },
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
Primitive::Primitive(const std::string &value)
    : PrimitiveType{std::make_shared<std::string>(value)} {}
Primitive::Primitive(std::string &&value)
    : PrimitiveType{std::make_shared<std::string>(std::move(value))} {}
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

} // namespace l3::vm
