#pragma once

#include "ast/ast.hpp"
#include "ast/nodes/literal.hpp"
#include "utils/debug.h"
#include "vm/types.hpp"
#include <cpptrace/from_current.hpp>
#include <stdexcept>
#include <utils/cow.h>

namespace l3::vm {

class VM {
public:
  VM() { scopes.emplace_back(); }

  void execute(const ast::Program &program);
  void execute(const ast::Statement &statement);
  void execute(const ast::Declaration &declaration);
  void execute(const ast::Assignment &assignment);

  void execute(const auto &node) {
    throw std::runtime_error(
        std::format("not implemented: {}", utils::debug::type_name(node))
    );
  }

  CowValue evaluate(const ast::Expression &expression);
  CowValue evaluate(const ast::Literal &literal);
  CowValue evaluate(const ast::Variable &variable);
  CowValue evaluate(const ast::BinaryExpression &binary);
  CowValue evaluate(const ast::Identifier &identifier);

  CowValue evaluate(const auto &node) {
    throw std::runtime_error(
        std::format("not implemented: {}", utils::debug::type_name(node))
    );
  }

private:
  Scope &current_scope() { return scopes.back(); }

  [[nodiscard]] std::optional<std::reference_wrapper<Value>>
  get_variable(const ast::Identifier &id) const;

  std::vector<Scope> scopes;
};

} // namespace l3::vm
