export module l3.compiler;

import std;
import l3.ast;
import l3.bytecode;
import l3.location;
import l3.runtime;
import utils;

using namespace l3::bytecode;

namespace l3::compiler {

export class CompileError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct Local {
  ast::Identifier name;
  int depth = -1;
};

struct Context {
  std::vector<Local> locals;
  std::vector<Upvalue> upvalues;
  std::size_t chunk_id;
  int scope_depth = 0;
  bool is_in_expression = false;
};

export class Compiler {
public:
  Compiler(ProgramBytecode &program);

  void compile(const ast::Program &program);

private:
  ProgramBytecode &program;
  Chunk &current_chunk();
  std::size_t last_instruction_offset();
  std::size_t current_instruction_offset();
  [[nodiscard]] const location::Location &current_location() const;

  std::vector<Context> contexts;
  std::vector<location::Location> location_stack;

  struct CompiledFunctionBody {
    std::vector<Upvalue> upvalues;
    std::size_t chunk_id;
  };

  enum class VariableType : std::uint8_t { Local, Upvalue, Global };
  struct ResolvedVariable {
    VariableType type;
    std::size_t index;
  };

  auto &&locals(this auto &&self);
  auto &&upvalues(this auto &&self);
  auto &&scope_depth(this auto &&self);

  ResolvedVariable resolve_variable(const ast::Identifier &identifier);
  Instruction emit_get_variable(const ast::Identifier &name);
  Instruction emit_set_variable(const ast::Identifier &name);

  std::size_t add_local(const ast::Identifier &name);

  struct LoopContext {
    std::vector<std::size_t> break_jumps;
    std::vector<std::size_t> continue_jumps;
    std::size_t body_locals_snapshot;
  };
  std::vector<LoopContext> loop_contexts;
  std::size_t synthetic_counter = 0;

  std::size_t push_context();
  void pop_context();

  void begin_scope();
  void end_scope(bool emit_pop = true);

  [[nodiscard]] std::optional<std::size_t>
  resolve_local(const ast::Identifier &name) const;
  std::optional<std::size_t> resolve_upvalue(const ast::Identifier &name);

  void emit(Instruction instruction);
  void emit(Instruction instruction, const location::Location &location);
  std::size_t make_constant(runtime::HeapData &&value = {});
  void emit_nil();
  void deduplicate_constants();
  void emit_loop(std::size_t loop_start);
  void patch_jump(std::size_t jump_offset, std::size_t target);
  void patch_jump_here(std::size_t jump_offset);
  std::size_t emit_jump(Instruction instruction);

  void compile_statements(std::ranges::input_range auto &statements);
  CompiledFunctionBody compile_function_body(const ast::FunctionBody &body);

  void compile_block(const ast::Block &block);
  void compile_expression(const ast::Expression &expr);
  void compile_variable(const ast::Variable &variable);
  void compile_literal(const ast::Literal &literal);
  void compile_statement(const ast::Statement &stmt);
  void compile_last_statement(const ast::LastStatement &stmt);

  // Extracted Expression Compilers
  void compile_binary_expression(const ast::BinaryExpression &binary);
  void compile_unary_expression(const ast::UnaryExpression &unary);
  void compile_logical_expression(const ast::LogicalExpression &logical);
  void compile_comparison(const ast::Comparison &comparison);
  void compile_anonymous_function(const ast::AnonymousFunction &func);
  void compile_function_call(const ast::FunctionCall &call);
  void compile_if_expression(const ast::IfExpression &if_expr);

  // Shared helper: compile a single condition+block pair and record the
  // jump-to-end. Used by both if-statement and if-expression.
  void compile_if_branch(
      const ast::Expression &condition,
      const ast::Block &block,
      std::vector<std::size_t> &end_jumps
  );

  // Extracted Statement Compilers
  void compile_declaration(const ast::Declaration &decl);
  void compile_for_loop(const ast::ForLoop &loop);
  void compile_function_call_statement(const ast::FunctionCall &call);
  void compile_if_statement(const ast::IfStatement &if_stmt);
  void compile_name_assignment(const ast::NameAssignment &assign);
  void compile_named_function(const ast::NamedFunction &func);
  void compile_operator_assignment(const ast::OperatorAssignment &assign);
  void compile_range_for_loop(const ast::RangeForLoop &loop);
  void compile_while_loop(const ast::While &loop);

  struct LoopPreamble {
    std::size_t control_jump;
    std::size_t body_offset;
  };
  [[nodiscard]] LoopPreamble begin_loop();
  void end_loop(LoopPreamble preamble, std::size_t control_offset);

  // Extracted LastStatement Compilers
  void compile_return_statement(const ast::ReturnStatement &ret);
  void compile_break_statement(const ast::BreakStatement &);
  void compile_continue_statement(const ast::ContinueStatement &);
};

} // namespace l3::compiler
