#include "vm/types.hpp"

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

void Scope::assign_variable(const ast::Identifier &id, Value &&value) {
  auto present = variables.find(id);
  if (present == variables.end()) {
    throw RuntimeError("variable '{}' not declared", id.name());
  }
  present->second = std::make_unique<Value>(std::move(value));
}

std::optional<std::reference_wrapper<Value>>
Scope::get_variable(const ast::Identifier &id) const {
  auto present = variables.find(id);
  if (present == variables.end()) {
    return std::nullopt;
  }
  return *present->second;
}

void Scope::declare_variable(
    const ast::Identifier &id, const std::function<CowValue()> &value
) {
  const auto present = variables.find(id);
  if (present != variables.end()) {
    throw RuntimeError("variable '{}' already declared", id.name());
  }
  variables.emplace_hint(
      present, id, std::make_unique<Value>(value().into_value())
  );
}

} // namespace l3::vm
