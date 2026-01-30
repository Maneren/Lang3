#include "vm/scope.hpp"
#include "vm/builtins.hpp"
#include "vm/error.hpp"

namespace l3::vm {

utils::optional_cref<Variable> Scope::get_variable(const Identifier &id) const {
  auto present = variables.find(id);
  if (present == variables.end()) {
    return std::nullopt;
  }
  return std::optional{std::cref(present->second)};
}
utils::optional_ref<Variable> Scope::get_variable_mut(const Identifier &id) {
  auto present = variables.find(id);
  if (present == variables.end()) {
    return std::nullopt;
  }
  return std::optional{std::ref(present->second)};
}

Variable &Scope::declare_variable(
    const Identifier &id, RefValue ref_value, Mutability mutability
) {
  const auto present = variables.find(id);
  if (present != variables.end()) {
    throw NameError("variable '{}' already declared", id.get_name());
  }
  auto &[_, value] =
      *variables.emplace_hint(present, id, Variable{ref_value, mutability});
  return value;
}

namespace {

std::pair<Identifier, GCValue>
wrap_native_function(std::string_view name, BuiltinFunction::Body function) {
  return {
      Identifier{name},
      GCValue{Value{BuiltinFunction{Identifier{name}, std::move(function)}}}
  };
}

Scope::BuiltinsMap create_builtins() {
  Scope::BuiltinsMap builtins;

  for (const auto &[name, value] : builtins::BUILTINS) {
    builtins.emplace(wrap_native_function(name, value));
  }

  return builtins;
}

} // namespace

Scope::BuiltinsMap Scope::builtins = create_builtins();

std::optional<RefValue> Scope::get_builtin(const Identifier &id) {
  auto present = builtins.find(id);
  if (present == builtins.end()) {
    return std::nullopt;
  }
  return RefValue{present->second};
}

Scope::Scope(VariableMap &&variables) : variables{std::move(variables)} {};
// const Scope &Scope::builtins() { return _builtins; }

void Scope::mark_gc() {
  for (auto &[name, it] : variables) {
    it->get_gc_mut().mark();
  }
}
bool Scope::has_variable(const Identifier &id) const {
  return variables.contains(id);
}

} // namespace l3::vm
