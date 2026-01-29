#pragma once

#include "vm/scope.hpp"
#include "vm/stack.hpp"
#include "vm/storage.hpp"
#include "vm/value.hpp"
#include <memory>
#include <print>
#include <vector>

import l3.ast;

namespace l3::vm {

class Scope;

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
  RefValue store_new_value(NewValue &&value);
  Variable &declare_variable(
      const Identifier &id, Mutability mut, GCValue &gc_value = GCStorage::nil()
  );
  static RefValue nil();

  size_t run_gc();

private:
  void execute(const ast::Block &block);
  void execute(const ast::Statement &statement);
  void execute(const ast::LastStatement &last_statement);
  void execute(const ast::Declaration &declaration);
  void execute(const ast::OperatorAssignment &assignment);
  void execute(const ast::NameAssignment &assignment);
  void execute(const ast::FunctionCall &function_call);
  void execute(const ast::IfStatement &if_statement);
  void execute(const ast::IfElseBase &if_else_base);
  bool execute(const ast::ElseIfList &elseif_list);
  void execute(const ast::NamedFunction &named_function);
  void execute(const ast::While &while_loop);

  [[nodiscard]] RefValue evaluate(const ast::Expression &expression);
  [[nodiscard]] RefValue evaluate(const ast::Literal &literal);
  [[nodiscard]] RefValue evaluate(const ast::Variable &variable);
  [[nodiscard]] RefValue evaluate(const ast::UnaryExpression &unary);
  [[nodiscard]] RefValue evaluate(const ast::BinaryExpression &binary);
  [[nodiscard]] RefValue evaluate(const ast::IndexExpression &index_ex);
  [[nodiscard]] RefValue evaluate(const ast::AnonymousFunction &anonymous);
  [[nodiscard]] RefValue evaluate(const ast::FunctionCall &function_call);
  [[nodiscard]] RefValue evaluate(const ast::IfExpression &if_expr);
  [[nodiscard]] RefValue evaluate(const ast::Identifier &identifier);

  [[nodiscard]] RefValue read_variable(const Identifier &id) const;
  [[nodiscard]] std::reference_wrapper<RefValue>
  read_write_variable(const Identifier &id);

  bool evaluate_if_branch(const ast::IfBase &if_base);

  bool debug;
  std::vector<std::shared_ptr<Scope>> scopes;
  std::vector<std::vector<std::shared_ptr<Scope>>> unused_scopes;
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
