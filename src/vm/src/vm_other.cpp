module l3.vm;

import utils;
import l3.ast;
import :gc_value;
import :formatting;
import :variable;

namespace l3::vm {

constexpr size_t GC_OBJECT_TRIGGER_TRESHOLD = 10000;

RefValue VM::read_variable(const Identifier &id) {
  debug_print("Reading variable {}", id.get_name());
  if (auto value = scopes->read_variable(id)) {
    return *value;
  }
  if (auto value = Scope::get_builtin(id)) {
    return *value;
  }
  throw UndefinedVariableError(id);
}

RefValue &VM::read_write_variable(const Identifier &id) {
  debug_print("Writing variable {}", id.get_name());
  if (auto value = scopes->read_variable_mut(id)) {
    return *value;
  }
  if (auto value = Scope::get_builtin(id)) {
    throw RuntimeError("Cannot modify builtin variable {}", id);
  }
  throw UndefinedVariableError(id);
}

VM::VM(bool debug)
    : debug{debug}, scopes{std::make_shared<ScopeStack>()}, stack{debug},
      gc_storage{debug} {}

size_t VM::run_gc() {
  const auto since_last_sweep = gc_storage.get_added_since_last_sweep();
  if (since_last_sweep < GC_OBJECT_TRIGGER_TRESHOLD) {
    debug_print("[GC] Skipping... only {} values", since_last_sweep);
    return 0;
  }

  debug_print("[GC] Running");
  scopes->mark_gc();
  stack.mark_gc();
  if (return_value) {
    return_value->get_gc_mut().mark();
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
    for (const auto &scope : scopes->get_scopes()) {
      debug_print("  {}", scope->get_variables().size());
    }
  }
  return erased;
}

Variable &VM::declare_variable(
    const Identifier &id, Mutability mutability, RefValue ref_value
) {
  debug_print(
      "Declaring {} variable {} = {}", mutability, id.get_name(), ref_value
  );
  return scopes->top().declare_variable(id, ref_value, mutability);
}

RefValue VM::store_value(Value &&value) {
  if (auto boolean = value.as_primitive().and_then(&Primitive::as_bool)) {
    if (*boolean) {
      return _true();
    }
    return _false();
  }

  auto ref_value = RefValue{gc_storage.emplace(std::move(value))};
  stack.push_value(ref_value);
  return ref_value;
}

RefValue VM::store_new_value(NewValue &&value) {
  return match::match(
      std::move(value),
      [this](Value &&value) { return store_value(std::move(value)); },
      [](RefValue value) { return value; }
  );
}

RefValue VM::nil() { return RefValue{GCStorage::nil()}; }
RefValue VM::_true() { return RefValue{GCStorage::_true()}; }
RefValue VM::_false() { return RefValue{GCStorage::_false()}; }

} // namespace l3::vm
