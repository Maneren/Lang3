#include "vm/value.hpp"
#include "vm/scope.hpp"
#include "vm/vm.hpp"

#include <ranges>

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

[[nodiscard]] Value Value::add(const Value &other) const {
  return binary_op(
      other, [](const auto &lhs, const auto &rhs) { return lhs + rhs; }, "add"
  );
}

[[nodiscard]] Value Value::sub(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs - rhs; },
      "subtract"
  );
}

[[nodiscard]] Value Value::mul(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs * rhs; },
      "multiply"
  );
}

[[nodiscard]] Value Value::div(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs / rhs; },
      "divide"
  );
}

[[nodiscard]] Value Value::mod(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs % rhs; },
      "modulo"
  );
}

[[nodiscard]] Value Value::and_op(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs && rhs; },
      "perform logical and on"
  );
}

[[nodiscard]] Value Value::or_op(const Value &other) const {
  return binary_op(
      other,
      [](const auto &lhs, const auto &rhs) { return lhs || rhs; },
      "perform logical or on"
  );
}

L3Function::L3Function(
    const std::vector<std::shared_ptr<Scope>> &active_scopes,
    const ast::NamedFunction &function
)
    : capture_scopes{active_scopes}, body{function.get_body()},
      name{function.get_name()} {}

CowValue Function::operator()(VM &vm, std::span<const CowValue> args) {
  return inner.visit([&vm, &args](auto &func) { return func(vm, args); });
};

[[nodiscard]] bool Primitive::as_bool() const {
  return visit(
      [](const bool &value) { return value; },
      [](const long long &value) { return value != 0; },
      [](const string_type &value) { return !value->empty(); },
      [](const double & /*value*/) -> bool {
        throw RuntimeError("cannot convert a floating point number to bool");
      }
  );
}
[[nodiscard]] bool Value::as_bool() const {
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

} // namespace l3::vm
