#pragma once

#include "vm/value.hpp"
#include <ast/nodes/identifier.hpp>

namespace l3::vm {

class Scope {
public:
  using VariableMap =
      std::unordered_map<ast::Identifier, std::shared_ptr<Value>>;
  Scope() = default;
  Scope(VariableMap &&variables);

  static const Scope &builtins();

  Value &declare_variable(const ast::Identifier &id);

  std::optional<std::reference_wrapper<const Value>>
  get_variable(const ast::Identifier &id) const;
  std::optional<std::reference_wrapper<Value>>
  get_variable(const ast::Identifier &id);

  const Scope::VariableMap &get_variables() const;

private:
  static Scope _builtins;
  VariableMap variables;
};

} // namespace l3::vm
