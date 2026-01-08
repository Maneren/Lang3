#pragma once

#include "vm/scope.hpp"
#include "vm/value.hpp"
#include <ast/ast.hpp>
#include <ast/printing.hpp>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <print>
#include <span>
#include <stdexcept>
#include <utils/debug.h>
#include <vector>

namespace l3::vm {

class VM {
public:
  VM(bool debug = false) : debug{debug} {}

  void execute(const ast::Program &program);
  void execute(const ast::Statement &statement);
  void execute(const ast::Declaration &declaration);
  void execute(const ast::Assignment &assignment);
  void execute(const ast::FunctionCall &function_call);
  void execute(const ast::IfStatement &if_statement);
  void execute(const ast::NamedFunction &named_function);

  void execute(const auto &node) {
    throw std::runtime_error(
        std::format(
            "execution not implemented: {}", utils::debug::type_name(node)
        )
    );
  }

  [[nodiscard]] CowValue evaluate(const ast::Block &block);
  [[nodiscard]] CowValue evaluate(const ast::LastStatement &last_statement);
  [[nodiscard]] CowValue evaluate(const ast::Expression &expression);
  [[nodiscard]] CowValue evaluate(const ast::Literal &literal) const;
  [[nodiscard]] CowValue evaluate(const ast::Variable &variable) const;
  [[nodiscard]] CowValue evaluate(const ast::BinaryExpression &binary);
  [[nodiscard]] CowValue evaluate(const ast::Identifier &identifier) const;
  [[nodiscard]] CowValue
  evaluate(const ast::AnonymousFunction &anonymous) const;
  [[nodiscard]] CowValue evaluate(const ast::FunctionCall &function_call);

  CowValue evaluate(const auto &node) const {
    throw std::runtime_error(
        std::format(
            "evaluation not implemented: {}", utils::debug::type_name(node)
        )
    );
  }

  CowValue evaluate_function_body(
      std::span<std::shared_ptr<Scope>> captured,
      Scope &&arguments,
      const ast::FunctionBody &body
  );

private:
  Scope &current_scope();

  [[nodiscard]] std::optional<std::reference_wrapper<const Value>>
  read_variable(const ast::Identifier &id) const;
  [[nodiscard]] std::optional<std::reference_wrapper<Value>>
  read_write_variable(const ast::Identifier &id);

  bool evaluate_if_branch(const ast::IfBase &if_base);

  bool debug;
  std::vector<std::shared_ptr<Scope>> scopes = {std::make_shared<Scope>()};

  template <typename... Ts>
  void debug_print(std::format_string<Ts...> message, Ts &&...args) const {
    if (debug) {
      std::println(std::cerr, message, std::forward<Ts>(args)...);
    }
  }
};

} // namespace l3::vm
