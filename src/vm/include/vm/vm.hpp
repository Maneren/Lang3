#pragma once

#include "vm/types.hpp"
#include <ast/ast.hpp>
#include <ast/printing.hpp>
#include <cpptrace/from_current.hpp>
#include <iostream>
#include <print>
#include <stdexcept>
#include <utils/cow.h>
#include <utils/debug.h>

namespace l3::vm {

class VM {
public:
  VM(bool debug = false) : debug{debug} {
    scopes.emplace_back(Scope::global());
  }

  void execute(const ast::Program &program);
  void execute(const ast::Statement &statement);
  void execute(const ast::Declaration &declaration);
  void execute(const ast::Assignment &assignment);
  void execute(const ast::FunctionCall &function_call);
  void execute(const ast::IfStatement &if_statement);

  void execute(const auto &node) {
    throw std::runtime_error(
        std::format(
            "execution not implemented: {}", utils::debug::type_name(node)
        )
    );
  }

  CowValue evaluate(const ast::Expression &expression);
  CowValue evaluate(const ast::Literal &literal);
  CowValue evaluate(const ast::Variable &variable);
  CowValue evaluate(const ast::BinaryExpression &binary);
  CowValue evaluate(const ast::Identifier &identifier);
  CowValue evaluate(const ast::FunctionCall &function_call);

  CowValue evaluate(const auto &node) {
    throw std::runtime_error(
        std::format(
            "evaluation not implemented: {}", utils::debug::type_name(node)
        )
    );
  }

private:
  Scope &current_scope() { return scopes.back(); }

  [[nodiscard]] std::optional<std::reference_wrapper<Value>>
  get_variable(const ast::Identifier &id) const;

  bool evaluate_if_branch(const ast::IfBase &if_base);

  bool debug;
  std::vector<Scope> scopes;

  template <typename... Ts>
  void debug_print(std::format_string<Ts...> message, Ts &&...args) const {
    if (debug) {
      std::println(std::cerr, message, std::forward<Ts>(args)...);
    }
  }
};

} // namespace l3::vm
