module l3.vm;

import utils;
import l3.ast;
import :gc_value;
import :formatting;
import :variable;

namespace l3::vm {

constexpr std::size_t GC_OBJECT_TRIGGER_TRESHOLD = 10000;

Ref VM::read_variable(const Identifier &id) {
  debug_print("Reading variable {}", id.get_name());
  if (auto value = state().scope_stack->read_variable(id)) {
    return *value;
  }
  if (auto value = Scope::get_builtin(id)) {
    return *value;
  }
  throw UndefinedVariableError(id);
}

Ref &VM::read_write_variable(const Identifier &id) {
  debug_print("Writing variable {}", id.get_name());
  if (auto value = state().scope_stack->read_variable_mut(id)) {
    return *value;
  }
  if (auto value = Scope::get_builtin(id)) {
    throw RuntimeError("Cannot modify builtin variable {}", id);
  }
  throw UndefinedVariableError(id);
}

VM::VM(bool debug)
    : debug{debug}, state_stack{ExecutionState{std::make_shared<ScopeStack>()}},
      stack{debug} {}

std::size_t VM::run_gc() {
  const auto since_last_sweep = gc_storage.get_added_since_last_sweep();
  if (since_last_sweep < GC_OBJECT_TRIGGER_TRESHOLD) {
    debug_print("[GC] Skipping... only {} values", since_last_sweep);
    return 0;
  }

  debug_print("[GC] Running");
  for (const auto &s : state_stack) {
    s.scope_stack->mark_gc();
  }
  stack.mark_gc();
  if (state().return_value) {
    state().return_value->get_gc_mut().mark();
  }
  auto erased = gc_storage.sweep();
  if (debug) {
    debug_print(
        "[GC] Swept {} values, keeping {}", erased, gc_storage.get_size()
    );
    debug_print("Stack:");
    for (const auto &frame : stack.get_frames()) {
      debug_print("  {}", frame.size());
    }
    debug_print("Scopes:");
    for (const auto &scope : state().scope_stack->get_scopes()) {
      debug_print("  {}", scope->get_variables().size());
    }
  }
  return erased;
}

Variable &VM::declare_variable(
    const Identifier &id, Mutability mutability, Ref ref_value
) {
  debug_print(
      "Declaring {} variable {} = {}", mutability, id.get_name(), ref_value
  );
  try {
    return state().scope_stack->declare_variable(id, ref_value, mutability);
  } catch (NameError &error) {
    error.set_location(id.get_location());
    throw;
  }
}

Ref VM::store_value(Value &&value) {
  if (auto boolean = value.as_primitive().and_then(&Primitive::as_bool)) {
    return *boolean ? _true() : _false();
  }

  if (value.is_nil()) {
    return nil();
  }

  auto ref_value = Ref{gc_storage.emplace(std::move(value))};
  stack.push_value(ref_value);
  return ref_value;
}

Ref VM::store_new_value(NewValue &&value) {
  return match::match(
      std::move(value),
      [this](Value &&value) { return store_value(std::move(value)); },
      [](Ref value) { return value; }
  );
}

Ref VM::nil() { return Ref{GCStorage::nil()}; }
Ref VM::_true() { return Ref{GCStorage::_true()}; }
Ref VM::_false() { return Ref{GCStorage::_false()}; }

} // namespace l3::vm
