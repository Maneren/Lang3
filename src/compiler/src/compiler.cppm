export module l3.compiler;

import std;
import l3.ast;
import l3.bytecode;
import l3.runtime;
import utils;

export namespace l3::compiler {

class Compiler {
public:
  Compiler(std::vector<bytecode::Chunk> &chunks);

  void compile(const ast::Program &program);

private:
  std::vector<bytecode::Chunk> &chunks;
  std::size_t current_chunk_id = 0;
  bytecode::Chunk &current_chunk();
  std::size_t last_instruction_offset();

  struct Local {
    std::string name;
    int depth;
  };
  std::vector<Local> locals;
  int scope_depth = 0;

  struct Upvalue {
    bool is_local;
    std::size_t index;
  };
  std::vector<Upvalue> current_upvalues;

  struct Context {
    std::size_t chunk_id;
    std::vector<Local> locals;
    int scope_depth;
    std::vector<Upvalue> upvalues;
  };
  std::vector<Context> contexts;

  std::vector<std::vector<std::size_t>> break_jumps_stack;
  std::vector<std::vector<std::size_t>> continue_jumps_stack;
  std::vector<std::size_t> loop_continues_stack;
  std::vector<std::size_t> loop_body_locals_snapshot;

  void push_context(std::size_t new_chunk_id);
  void pop_context();

  void begin_scope();
  void end_scope();

  int resolve_local(const std::string &name);
  int resolve_local_in_context(const std::string &name, const Context &ctx);
  int add_upvalue(
      std::vector<Upvalue> &upvalues, bool is_local, std::size_t index
  );
  int resolve_upvalue(const std::string &name, int context_index);

  void emit(const bytecode::Instruction &instruction, std::size_t line = 0);
  std::size_t make_constant(runtime::Value &&value);
  void emit_loop(std::size_t loop_start);
  void patch_jump(std::size_t offset);
  std::size_t emit_jump(const bytecode::Instruction &instruction);

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

  // Extracted LastStatement Compilers
  void compile_return_statement(const ast::ReturnStatement &ret);
  void compile_break_statement(const ast::BreakStatement &);
  void compile_continue_statement(const ast::ContinueStatement &);
};

} // namespace l3::compiler
