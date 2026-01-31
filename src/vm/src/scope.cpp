module;

#include <memory>
#include <ranges>
#include <utility>

module l3.vm;

import utils;
import :builtins;
import :gc_value;
import :variable;
import :formatting;

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
      GCValue{Value{BuiltinFunction{Identifier{name}, function}}}
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

void Scope::mark_gc() {
  for (auto &[_, it] : variables) {
    it->get_gc_mut().mark();
  }
}
bool Scope::has_variable(const Identifier &id) const {
  return variables.contains(id);
}

size_t Scope::size() const { return variables.size(); }

Scope Scope::clone(VM &vm) const {
  Scope cloned;
  cloned.variables.reserve(size());
  for (const auto &[name, var] : variables) {
    cloned.declare_variable(
        name, vm.store_value(var.get()->clone()), var.get_mutability()
    );
  }
  return cloned;
}

ScopeStack ScopeStack::clone(VM &vm) const {
  ScopeStack cloned;
  cloned.scopes.reserve(size());
  for (const auto &scope : scopes) {
    cloned.push_back(std::make_shared<Scope>(scope->clone(vm)));
  }
  return cloned;
}
ScopeStack::FrameGuard::FrameGuard(ScopeStack &scope_stack)
    : scope_stack{scope_stack} {
  scope_stack.push_back(std::make_shared<Scope>());
}
ScopeStack::FrameGuard::FrameGuard(ScopeStack &scope_stack, Scope &&scope)
    : scope_stack{scope_stack} {
  scope_stack.push_back(std::make_shared<Scope>(std::move(scope)));
}
ScopeStack::FrameGuard::~FrameGuard() { scope_stack.scopes.pop_back(); }
void ScopeStack::pop_back() { scopes.pop_back(); }
void ScopeStack::push_back(std::shared_ptr<Scope> &&scope) {
  scopes.push_back(scope);
}

const Scope &ScopeStack::top() const { return *scopes.back(); }
Scope &ScopeStack::top() { return *scopes.back(); }
size_t ScopeStack::size() const { return scopes.size(); }

ScopeStack::FrameGuard ScopeStack::with_frame() {
  return FrameGuard(*this);
}
ScopeStack::FrameGuard ScopeStack::with_frame(Scope &&scope) {
  return FrameGuard(*this, std::move(scope));
}

[[nodiscard]]
std::optional<RefValue> ScopeStack::read_variable(const Identifier &id) const {
  for (const auto &scope : std::views::reverse(scopes)) {
    if (auto variable = scope->get_variable(id)) {
      return *variable->get();
    }
  }
  return std::nullopt;
}

[[nodiscard]]
utils::optional_ref<RefValue>
ScopeStack::read_variable_mut(const Identifier &id) {
  for (auto &scope : std::views::reverse(scopes)) {
    if (auto variable = scope->get_variable_mut(id)) {
      if (variable->get().is_const()) {
        throw RuntimeError("Cannot modify constant variable {}", id);
      }
      return *variable->get();
    }
  }
  return std::nullopt;
}

} // namespace l3::vm
