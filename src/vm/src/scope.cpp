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
  return std::optional{std::cref(present->second.get())};
}
std::optional<RefValue> Scope::get_variable(const ast::Identifier &id) {
  auto present = variables.find(id);
  if (present == variables.end()) {
    return std::nullopt;
  }
  return std::optional{present->second};
}

void Scope::declare_variable(const ast::Identifier &id, GCValue &gc_value) {
  const auto present = variables.find(id);
  if (present != variables.end()) {
    throw NameError("variable '{}' already declared", id.name());
  }
  variables.emplace_hint(present, id, std::ref(gc_value));
}

namespace {

std::pair<ast::Identifier, std::shared_ptr<Value>>
wrap_native_function(std::string_view name, BuiltinFunction::Body function) {
  auto function_ptr = std::make_shared<Value>(
      BuiltinFunction{ast::Identifier{name}, std::move(function)}
  );

  return {ast::Identifier{name}, function_ptr};
}

RefValue print(VM &vm, L3Args args) {
  std::print("{}", args[0].get());
  for (const auto &arg : args | std::views::drop(1)) {
    std::print(" {}", arg.get());
  }
  return vm.store_value(Value{});
}

RefValue println(VM &vm, L3Args args) {
  print(vm, args);
  std::print("\n");
  return vm.store_value(Value{});
}

RefValue trigger_gc(VM &vm, L3Args args) {
  if (args.size() > 0) {
    throw RuntimeError("trigger_gc takes no arguments");
  }
  vm.run_gc();
  return vm.store_value(Value{});
}

RefValue l3_assert(VM &vm, L3Args args) {
  if (args[0]->as_bool()) {
    return vm.store_value(Value{});
  }
  throw RuntimeError("{}", args[0]);
}

Scope::BuiltinsMap create_builtins() {
  return {
      wrap_native_function("print", print),
      wrap_native_function("println", println),
      wrap_native_function(
          "error",
          [](VM & /*vm*/, L3Args args) -> RefValue {
            throw RuntimeError("{}", args[0]);
          }
      ),
      wrap_native_function("assert", l3_assert),
      wrap_native_function("__trigger_gc", trigger_gc)
  };
}

} // namespace

Scope::BuiltinsMap Scope::_builtins = create_builtins();

Scope::Scope(VariableMap &&variables) : variables{std::move(variables)} {};
const Scope::BuiltinsMap &Scope::builtins() { return _builtins; }
const Scope::VariableMap &Scope::get_variables() const { return variables; }

void Scope::mark_gc() {
  for (auto &[name, it] : variables) {
    it.get_gc().mark();
  }
}
bool Scope::has_variable(const ast::Identifier &id) const {
  return variables.contains(id);
}

} // namespace l3::vm
