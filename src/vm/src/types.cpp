#include "vm/types.hpp"
#include <print>
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
      []<typename T, typename U>(const T &lhs, const U &rhs) -> Primitive
        requires std::is_same_v<T, U> &&
                 requires(T lhs, U rhs) { lhs / rhs; } &&
                 (!std::is_same_v<T, bool> || !std::is_same_v<U, bool>)
      {
        if (!rhs) {
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
      []<typename T, typename U>(const T &lhs, const U &rhs) -> Primitive
        requires std::is_same_v<T, U> &&
                 requires(T lhs, U rhs) { lhs % rhs; } &&
                 (!std::is_same_v<T, bool> || !std::is_same_v<U, bool>)
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
      },
      []<typename T, typename U>(const T &lhs, const U &rhs) -> Primitive
        requires std::is_same_v<T, U> && requires(T lhs, U rhs) { lhs && rhs; }
      { return Primitive{lhs && rhs}; }
      );
}

Primitive operator||(const Primitive &lhs, const Primitive &rhs) {
  return handle_op(
      "logical disjunction (or)",
      lhs,
      rhs,
      [](const bool &lhs, const bool &rhs) -> Primitive {
        return Primitive{lhs || rhs};
      },
      []<typename T, typename U>(const T &lhs, const U &rhs) -> Primitive
        requires std::is_same_v<T, U> && requires(T lhs, U rhs) { lhs || rhs; }
      { return Primitive{lhs || rhs}; }
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

std::optional<std::reference_wrapper<Value>>
Scope::get_variable(const ast::Identifier &id) const {
  auto present = variables.find(id);
  if (present == variables.end()) {
    return std::nullopt;
  }
  return *present->second;
}

Value &Scope::declare_variable(const ast::Identifier &id) {
  const auto present = variables.find(id);
  if (present != variables.end()) {
    throw NameError("variable '{}' already declared", id.name());
  }
  const auto &[_, inserted] =
      *variables.emplace_hint(present, id, std::make_unique<Value>());

  return *inserted;
}

Scope Scope::global() {
  Scope scope;
  scope.declare_variable(ast::Identifier{"print"}) =
      Value{BuiltinFunction{[](std::span<const CowValue> args) {
        std::print("{}", args[0]);
        for (const auto &arg : args | std::views::drop(1)) {
          std::print(" {}", arg);
        }
        return CowValue{Value{Nil{}}};
      }}};
  scope.declare_variable(ast::Identifier{"println"}) =
      Value{BuiltinFunction{[](std::span<const CowValue> args) {
        std::print("{}", args[0]);
        for (const auto &arg : args | std::views::drop(1)) {
          std::print(" {}", arg);
        }
        std::print("\n");
        return CowValue{Value{Nil{}}};
      }}};
  return scope;
}

L3Function::L3Function(Scope &&capture_scope, ast::FunctionBody body)
    : capture_scope{std::make_unique<Scope>(std::move(capture_scope))},
      body{std::move(body)} {}

CowValue Function::operator()(std::span<const CowValue> args) const {
  return inner.visit([&args](const auto &func) { return func(args); });
};

} // namespace l3::vm
