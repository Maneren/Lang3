#pragma once

#include "vm/identifier.hpp"
#include "vm/ref_value.hpp"
#include "vm/storage.hpp"
#include "vm/value.hpp"
#include <utils/accessor.h>

namespace l3::vm {

using Mutability = ast::Mutability;

class Variable {
public:
  Variable() = delete;
  Variable(RefValue ref_value, Mutability mut)
      : ref_value(std::move(ref_value)), mut(mut) {}

  [[nodiscard]] const RefValue &get() const { return ref_value; }
  [[nodiscard]] RefValue &get() { return ref_value; }

  [[nodiscard]] RefValue &operator*() { return get(); }
  [[nodiscard]] const RefValue &operator*() const { return get(); }

  RefValue *operator->() { return &get(); }
  const RefValue *operator->() const { return &get(); }

  DEFINE_ACCESSOR(mutability, Mutability, mut);

  [[nodiscard]] bool is_const() const { return mut == Mutability::Immutable; }
  [[nodiscard]] bool is_mutable() const { return mut == Mutability::Mutable; }

  RefValue ref_value;
  Mutability mut;
};

class Scope {
public:
  using VariableMap = std::unordered_map<Identifier, Variable>;
  using BuiltinsMap = std::unordered_map<Identifier, GCValue>;
  Scope() = default;
  Scope(VariableMap &&variables);

  // static const Scope &builtins();

  Variable &
  declare_variable(const Identifier &id, RefValue gc_value, Mutability mut);

  utils::optional_cref<Variable> get_variable(const Identifier &id) const;
  utils::optional_ref<Variable> get_variable_mut(const Identifier &id);

  bool has_variable(const Identifier &id) const;

  DEFINE_ACCESSOR(variables, VariableMap, variables);

  void mark_gc();

  static std::optional<RefValue> get_builtin(const Identifier &id);

private:
  static BuiltinsMap builtins;
  VariableMap variables;
};

} // namespace l3::vm
