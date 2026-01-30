#pragma once

#include "vm/identifier.hpp"
#include "vm/ref_value.hpp"
#include "vm/storage.hpp"
#include "vm/value.hpp"
#include <utils/accessor.h>

namespace l3::vm {

using Mutability = ast::Mutability;

class Variable {
  RefValue ref_value;
  Mutability mutability;

public:
  Variable() = delete;
  Variable(RefValue ref_value, Mutability mutability)
      : ref_value(std::move(ref_value)), mutability(mutability) {}

  [[nodiscard]] const RefValue &get() const { return ref_value; }
  [[nodiscard]] RefValue &get() { return ref_value; }

  [[nodiscard]] RefValue &operator*() { return get(); }
  [[nodiscard]] const RefValue &operator*() const { return get(); }

  RefValue *operator->() { return &get(); }
  const RefValue *operator->() const { return &get(); }

  DEFINE_VALUE_ACCESSOR_X(mutability);

  [[nodiscard]] bool is_const() const {
    return mutability == Mutability::Immutable;
  }
  [[nodiscard]] bool is_mutable() const {
    return mutability == Mutability::Mutable;
  }
};

class VM;

class Scope {
public:
  using VariableMap = std::unordered_map<Identifier, Variable>;
  using BuiltinsMap = std::unordered_map<Identifier, GCValue>;

private:
  static BuiltinsMap builtins;
  VariableMap variables;

public:
  Scope() = default;
  Scope(VariableMap &&variables);

  Variable &declare_variable(
      const Identifier &id, RefValue gc_value, Mutability mutability
  );

  utils::optional_cref<Variable> get_variable(const Identifier &id) const;
  utils::optional_ref<Variable> get_variable_mut(const Identifier &id);

  bool has_variable(const Identifier &id) const;
  size_t size() const { return variables.size(); }

  DEFINE_ACCESSOR_X(variables);

  void mark_gc();

  static std::optional<RefValue> get_builtin(const Identifier &id);

  [[nodiscard]] Scope clone(VM &vm) const;
};

class ScopeStack : public std::vector<std::shared_ptr<Scope>> {
public:
  [[nodiscard]] const Scope &top() const { return *back(); }
  [[nodiscard]] Scope &top() { return *back(); }

  [[nodiscard]] ScopeStack clone(VM &vm) const;
};

} // namespace l3::vm
