#pragma once

#include "utils/accessor.h"
#include "vm/storage.hpp"
#include "vm/value.hpp"
#include <ast/nodes/identifier.hpp>

namespace l3::vm {

class Scope {
public:
  using Variable = std::reference_wrapper<GCValue>;
  using VariableMap = std::unordered_map<ast::Identifier, RefValue>;
  using BuiltinsMap =
      std::unordered_map<ast::Identifier, std::shared_ptr<Value>>;
  Scope() = default;
  Scope(VariableMap &&variables);

  static const BuiltinsMap &builtins();

  RefValue &declare_variable(const ast::Identifier &id, GCValue &gc_value);

  utils::optional_cref<Value> get_variable(const ast::Identifier &id) const;
  utils::optional_ref<RefValue> get_variable(const ast::Identifier &id);

  bool has_variable(const ast::Identifier &id) const;

  DEFINE_ACCESSOR(variables, VariableMap, variables);

  void mark_gc();

private:
  static BuiltinsMap _builtins;
  VariableMap variables;
};

} // namespace l3::vm
