#include "vm/scope.hpp"
#include "vm/format.hpp"
#include "vm/function.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"
#include <ast/nodes/identifier.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <string_view>
#include <utility>

namespace l3::vm {

std::optional<std::reference_wrapper<const Value>>
Scope::get_variable(const ast::Identifier &id) const {
  auto present = variables.find(id);
  if (present == variables.end()) {
    return std::nullopt;
  }
  return std::optional{std::cref(*present->second)};
}
std::optional<std::reference_wrapper<Value>>
Scope::get_variable(const ast::Identifier &id) {
  auto present = variables.find(id);
  if (present == variables.end()) {
    return std::nullopt;
  }
  return std::optional{std::ref(*present->second)};
}

Value &Scope::declare_variable(const ast::Identifier &id) {
  const auto present = variables.find(id);
  if (present != variables.end()) {
    throw NameError("variable '{}' already declared", id.name());
  }
  auto &[_, inserted] =
      *variables.emplace_hint(present, id, std::make_shared<Value>());

  return *inserted;
}

namespace {

std::pair<ast::Identifier, std::shared_ptr<Value>>
wrap_native_function(std::string_view name, BuiltinFunction::Body function) {
  auto function_ptr = std::make_shared<Value>(
      BuiltinFunction{ast::Identifier{name}, std::move(function)}
  );

  return {ast::Identifier{name}, function_ptr};
}

Scope create_builtins() {
  return {{wrap_native_function(
    "print", 
    [](VM & /*vm*/, std::span<const CowValue> args) {
      std::print("{}", args[0]);
      for (const auto &arg : args | std::views::drop(1)) {
        std::print(" {}", arg);
      }
      return CowValue{Value{}};
    }
  ), 
    wrap_native_function("println", [](VM & /*vm*/, std::span<const CowValue> args) {
      std::print("{}", args[0]);
      for (const auto &arg : args | std::views::drop(1)) {
        std::print(" {}", arg);
      }
      std::print("\n");
      return CowValue{Value{}};
    })

  }};
}

} // namespace

Scope Scope::_builtins = create_builtins();

Scope::Scope(VariableMap &&variables) : variables{std::move(variables)} {};
const Scope &Scope::builtins() { return _builtins; }
const Scope::VariableMap &Scope::get_variables() const { return variables; }

} // namespace l3::vm
