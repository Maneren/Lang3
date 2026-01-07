#pragma once

#include "ast/nodes/identifier.hpp"
#include "vm/value.hpp"

namespace l3::vm {

class Scope {
public:
  using VariableMap =
      std::unordered_map<ast::Identifier, std::shared_ptr<Value>>;
  Scope() = default;
  Scope(VariableMap &&variables) : variables{std::move(variables)} {};

  static const Scope &builtins() { return _builtins; }

  Value &declare_variable(const ast::Identifier &id);

  std::optional<std::reference_wrapper<const Value>>
  get_variable(const ast::Identifier &id) const;
  std::optional<std::reference_wrapper<Value>>
  get_variable(const ast::Identifier &id);

  const auto &get_variables() const { return variables; }

private:
  static Scope _builtins;
  VariableMap variables;
};

} // namespace l3::vm
