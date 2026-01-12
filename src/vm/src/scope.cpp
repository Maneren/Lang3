#include "vm/scope.hpp"
#include "vm/error.hpp"
#include "vm/format.hpp"
#include "vm/function.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"
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

CowValue print(VM & /*vm*/, std::span<const CowValue> args) {
  std::print("{}", args[0]);
  for (const auto &arg : args | std::views::drop(1)) {
    std::print(" {}", arg);
  }
  return CowValue{Value{}};
}

CowValue println(VM &vm, std::span<const CowValue> args) {
  print(vm, args);
  std::print("\n");
  return CowValue{Value{}};
}

CowValue l3_assert(VM & /*vm*/, std::span<const CowValue> args) {
  if (args[0]->as_bool()) {
    return CowValue{Value{}};
  }
  throw RuntimeError("{}", args[0]);
}

Scope create_builtins() {
  return Scope{
      {wrap_native_function("print", print),
       wrap_native_function("println", println),
       wrap_native_function(
           "error",
           [](VM & /*vm*/, std::span<const CowValue> args) -> CowValue {
             throw RuntimeError("{}", args[0]);
           }
       ),
       wrap_native_function("assert", l3_assert)}
  };
}

} // namespace

Scope Scope::_builtins = create_builtins();

Scope::Scope(VariableMap &&variables) : variables{std::move(variables)} {};
const Scope &Scope::builtins() { return _builtins; }
const Scope::VariableMap &Scope::get_variables() const { return variables; }

} // namespace l3::vm
