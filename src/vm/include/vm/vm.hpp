#pragma once

#include "vm/scope.hpp"
#include "vm/storage.hpp"
#include "vm/value.hpp"
#include <ast/ast.hpp>
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

class Stack {
public:
  std::vector<RefValue> &top_frame();
  void push_frame();
  void pop_frame();
  RefValue push_value(RefValue value);

  void mark_gc();

private:
  std::vector<std::vector<RefValue>> frames{{}};
};

class VM {
public:
  VM(bool debug = false);

  void execute(const ast::Program &program);

  RefValue evaluate_function_body(
      std::span<std::shared_ptr<Scope>> captured,
      Scope &&arguments,
      const ast::FunctionBody &body
  );

  RefValue store_value(Value &&value);
  RefValue declare_variable(const ast::Identifier &id);

  size_t run_gc();

private:
  void execute(const ast::Block &block);
  void execute(const ast::Statement &statement);
  void execute(const ast::LastStatement &last_statement);
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

  [[nodiscard]] RefValue evaluate(const ast::Expression &expression);
  [[nodiscard]] RefValue evaluate(const ast::Literal &literal);
  [[nodiscard]] RefValue evaluate(const ast::Variable &variable);
  [[nodiscard]] RefValue evaluate(const ast::BinaryExpression &binary);
  [[nodiscard]] RefValue evaluate(const ast::Identifier &identifier);
  [[nodiscard]] RefValue evaluate(const ast::AnonymousFunction &anonymous);
  [[nodiscard]] RefValue evaluate(const ast::FunctionCall &function_call);

  RefValue evaluate(const auto &node) const {
    throw std::runtime_error(
        std::format(
            "evaluation not implemented: {}", utils::debug::type_name(node)
        )
    );
  }

  Scope &current_scope();

  [[nodiscard]] std::optional<std::reference_wrapper<const Value>>
  read_variable(const ast::Identifier &id) const;
  [[nodiscard]] std::optional<RefValue>
  read_write_variable(const ast::Identifier &id);

  bool evaluate_if_branch(const ast::IfBase &if_base);

  bool debug;
  std::vector<std::shared_ptr<Scope>> scopes;
  Stack stack;
  GCStorage gc_storage;

  template <typename... Ts>
  void debug_print(std::format_string<Ts...> message, Ts &&...args) const {
    if (debug) {
      std::println(std::cerr, message, std::forward<Ts>(args)...);
    }
  }

  struct BreakFlowException {
    std::optional<RefValue> value;
  };
};

} // namespace l3::vm
