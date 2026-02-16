module l3.vm;

import utils;
import :builtins;
import :gc_value;
import :variable;
import :formatting;

namespace l3::vm {

utils::optional_cref<Variable> Scope::get_variable(const Identifier &id) const {
  for (const auto &[name, var] : variables) {
    if (name == id) {
      return std::cref(var);
    }
  }
  return std::nullopt;
}
utils::optional_ref<Variable> Scope::get_variable_mut(const Identifier &id) {
  for (auto &[name, var] : variables) {
    if (name == id) {
      return std::ref(var);
    }
  }
  return std::nullopt;
}

Variable &Scope::declare_variable(
    const Identifier &id, Ref ref_value, Mutability mutability
) {
  if (get_variable(id)) {
    throw NameError("variable '{}' already declared", id.get_name());
  }
  auto &[_, value] =
      variables.emplace_back(id, Variable{ref_value, mutability});
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
    builtins.emplace_back(wrap_native_function(name, value));
  }

  return builtins;
}

} // namespace

Scope::BuiltinsMap Scope::builtins = create_builtins();

std::optional<Ref> Scope::get_builtin(const Identifier &id) {
  for (auto &[name, ref] : builtins) {
    if (name == id) {
      return Ref{ref};
    }
  }
  return std::nullopt;
}

Scope::Scope() = default;
Scope::Scope(VariableMap &&variables) : variables{std::move(variables)} {};

void Scope::mark_gc() {
  for (auto &[_, var] : variables) {
    var->get_gc_mut().mark();
  }
}
bool Scope::has_variable(const Identifier &id) const {
  return get_variable(id).has_value();
}

std::size_t Scope::size() const { return variables.size(); }

Scope Scope::clone(VM &vm) const {
  Scope cloned;
  cloned.variables.reserve(size());
  for (const auto &[name, var] : variables) {
    const auto value_ref = var.get();
    const auto cloned_value = value_ref->as_primitive()
                                  .transform([&vm](Primitive primitive) -> Ref {
                                    return vm.store_value(primitive);
                                  })
                                  .value_or(value_ref);

    cloned.variables.emplace_back(
        name, Variable{cloned_value, var.get_mutability()}
    );
  }
  return cloned;
}

ScopeStack ScopeStack::clone(VM &vm) const {
  ScopeStack cloned;
  cloned.scopes.reserve(size());
  for (const auto &scope : scopes) {
    cloned.scopes.push_back(std::make_shared<Scope>(scope->clone(vm)));
  }
  return cloned;
}
ScopeStack::FrameGuard::FrameGuard(ScopeStack &scope_stack)
    : scope_stack{scope_stack} {
  scope_stack.scopes.push_back(std::make_shared<Scope>());
}
ScopeStack::FrameGuard::FrameGuard(ScopeStack &scope_stack, Scope &&scope)
    : scope_stack{scope_stack} {
  scope_stack.scopes.push_back(std::make_shared<Scope>(std::move(scope)));
}
ScopeStack::FrameGuard::~FrameGuard() { scope_stack.scopes.pop_back(); }

const Scope &ScopeStack::top() const { return *scopes.back(); }
Scope &ScopeStack::top() { return *scopes.back(); }
std::size_t ScopeStack::size() const { return scopes.size(); }

ScopeStack::FrameGuard ScopeStack::with_frame() { return FrameGuard(*this); }
ScopeStack::FrameGuard ScopeStack::with_frame(Scope &&scope) {
  return FrameGuard(*this, std::move(scope));
}

[[nodiscard]]
std::optional<Ref> ScopeStack::read_variable(const Identifier &id) const {
  for (const auto &scope : std::views::reverse(scopes)) {
    if (auto variable = scope->get_variable(id)) {
      return *variable->get();
    }
  }
  return std::nullopt;
}

[[nodiscard]]
utils::optional_ref<Ref> ScopeStack::read_variable_mut(const Identifier &id) {
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

Variable &ScopeStack::declare_variable(
    const Identifier &id, Ref ref, Mutability mutability
) {
  return top().declare_variable(id, ref, mutability);
}

void ScopeStack::mark_gc() {
  for (auto &scope : scopes) {
    scope->mark_gc();
  }
}

} // namespace l3::vm
