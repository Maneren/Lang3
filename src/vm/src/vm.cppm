module;

#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <print>
#include <utility>
#include <vector>

export module l3.vm;

import utils;
import :error;
import :function;
import :identifier;
import :ref_value;
import :scope;
import :stack;
import :storage;
import :value;

export namespace l3::vm {

class VM {
public:
  VM(bool debug = false);

  void execute(const ast::Program &program);

  RefValue evaluate_function_body(
      const std::shared_ptr<ScopeStack> &captures,
      Scope &&arguments,
      const ast::FunctionBody &body
  );

  RefValue store_value(Value &&value);
  RefValue store_new_value(NewValue &&value);
  Variable &declare_variable(
      const Identifier &id,
      Mutability mutability,
      RefValue ref_value = RefValue{GCStorage::nil()}
  );
  static RefValue nil();
  static RefValue _true();
  static RefValue _false();

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
  void execute(const ast::ForLoop &for_loop);
  void execute(const ast::RangeForLoop &range_for_loop);

  [[nodiscard]] RefValue evaluate(const ast::Expression &expression);
  [[nodiscard]] RefValue evaluate(const ast::Literal &literal);
  [[nodiscard]] RefValue evaluate(const ast::Variable &variable);
  [[nodiscard]] RefValue evaluate(const ast::UnaryExpression &unary);
  [[nodiscard]] RefValue evaluate(const ast::BinaryExpression &binary);
  [[nodiscard]] RefValue evaluate(const ast::LogicalExpression &logical);
  [[nodiscard]] RefValue evaluate(const ast::Comparison &chained);
  [[nodiscard]] RefValue evaluate(const ast::IndexExpression &index_expression);
  [[nodiscard]] RefValue evaluate(const ast::AnonymousFunction &anonymous);
  [[nodiscard]] RefValue evaluate(const ast::FunctionCall &function_call);
  [[nodiscard]] RefValue evaluate(const ast::IfExpression &if_expr);
  [[nodiscard]] RefValue evaluate(const ast::Identifier &identifier);

  [[nodiscard]] RefValue &evaluate_mut(const ast::Variable &variable);
  [[nodiscard]] RefValue &evaluate_mut(const ast::Identifier &identifier);
  [[nodiscard]] RefValue &
  evaluate_mut(const ast::IndexExpression &index_expression);

  [[nodiscard]] RefValue read_variable(const Identifier &id);
  [[nodiscard]] RefValue &read_write_variable(const Identifier &id);

  bool evaluate_if_branch(const ast::IfBase &if_base);

  bool debug;
  std::shared_ptr<ScopeStack> scopes;
  std::vector<std::shared_ptr<ScopeStack>> unused_scopes;
  Stack stack;
  GCStorage gc_storage;

  enum class FlowControl : std::uint_fast8_t {
    Normal,
    Return,
    Break,
    Continue
  };
  friend std::formatter<FlowControl>;

  FlowControl flow_control = FlowControl::Normal;
  std::optional<RefValue> return_value = std::nullopt;

  template <typename... Ts>
  void debug_print(std::format_string<Ts...> message, Ts &&...args) const {
    if (debug) [[unlikely]] {
      std::println(std::cerr, message, std::forward<Ts>(args)...);
    }
  }

  class ScopeStackOverlay {
    VM &vm; // NOLINT

  public:
    ScopeStackOverlay(const ScopeStackOverlay &) = delete;
    ScopeStackOverlay(ScopeStackOverlay &&) = delete;
    ScopeStackOverlay &operator=(const ScopeStackOverlay &) = delete;
    ScopeStackOverlay &operator=(ScopeStackOverlay &&) = delete;

    explicit ScopeStackOverlay(
        VM &vm, std::shared_ptr<ScopeStack> overlay_scopes
    )
        : vm{vm} {
      vm.unused_scopes.emplace_back(std::move(vm.scopes));
      vm.scopes = std::move(overlay_scopes);
    }
    ~ScopeStackOverlay() {
      vm.scopes = std::move(vm.unused_scopes.back());
      vm.unused_scopes.pop_back();
    }
  };
};

} // namespace l3::vm

export {
  template <>
  struct std::formatter<l3::vm::VM::FlowControl>
      : utils::static_formatter<l3::vm::VM::FlowControl> {
    static auto format(auto obj, std::format_context &ctx) {
      switch (obj) {
        using namespace l3::vm;
      case VM::FlowControl::Normal:
        return std::format_to(ctx.out(), "normal");
      case VM::FlowControl::Break:
        return std::format_to(ctx.out(), "break");
      case VM::FlowControl::Continue:
        return std::format_to(ctx.out(), "continue");
      case VM::FlowControl::Return:
        return std::format_to(ctx.out(), "return");
      }
    }
  };
}
