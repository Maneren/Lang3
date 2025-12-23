#pragma once

#include "ast/ast.hpp"
#include "ast/nodes/literal.hpp"
#include "vm/types.hpp"
#include <cpptrace/from_current.hpp>
#include <ranges>
#include <stdexcept>
#include <utils/cow.h>

namespace l3::vm {

class VM {
public:
  VM() { scopes.emplace_back(); }

  void execute(const ast::Program &program);
  void execute(const ast::Statement &statement);
  void execute(const ast::Declaration &declaration);

  void execute(const auto & /*unused*/) {
    throw std::runtime_error("not implemented");
  }

  CowValue evaluate(const ast::Expression &expression);
  CowValue evaluate(const ast::Literal &literal);
  CowValue evaluate(const ast::BinaryExpression &binary);
  CowValue evaluate(const ast::Identifier &identifier);

  CowValue evaluate(const auto &node) {
    throw std::runtime_error(
        std::format("not implemented: {}", typeid(node).name())
    );
  }

private:
  Scope &current_scope() { return scopes.back(); }

  [[nodiscard]] std::optional<CowValue>
  get_variable(const ast::Identifier &id) const {
    for (const auto &it : std::views::reverse(scopes)) {
      if (auto value = it.get_variable(id)) {
        return CowValue{*value};
      }
    }
    return std::nullopt;
  }

  std::vector<Scope> scopes;
};

} // namespace l3::vm
