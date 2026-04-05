export module l3.vm;

import std;
import l3.bytecode;
import l3.runtime;
import utils;
import :builtins;

export namespace l3::vm {

class BytecodeVM {
public:
  explicit BytecodeVM(bool debug_ = false);

  runtime::Ref store_value(runtime::Value &&value);
  runtime::Ref store_new_value(runtime::NewValue &&value);
  runtime::Ref evaluate(runtime::Ref function, runtime::L3Args arguments);

  std::size_t run_gc();
  void maybe_gc();

  struct CallFrame {
    std::size_t chunk_id = 0;
    std::size_t ip = 0;
    std::size_t frame_pointer = 0;
    std::optional<runtime::Ref> closure;
  };

  void execute(const std::vector<bytecode::Chunk> &chunks);

private:
  std::optional<std::size_t>
  resolve_file_scope_slot(std::string_view name) const;
  std::size_t define_file_scope_slot(std::string_view name, runtime::Ref value);
  runtime::Ref &file_scope_slot(std::size_t index);
  const runtime::Ref &file_scope_slot(std::size_t index) const;

  void assign_slot(runtime::Ref &slot);
  void mutate_slot(runtime::Ref &slot);

  void execute_loop(std::size_t target_frames);

  void execute_op_return(const bytecode::OpReturn &op);
  void execute_op_constant(
      const bytecode::OpConstant &op, const bytecode::Chunk &chunk
  );
  void execute_op_pop(const bytecode::OpPop &op);
  void execute_op_add(const bytecode::OpAdd &op);
  void execute_op_subtract(const bytecode::OpSubtract &op);
  void execute_op_multiply(const bytecode::OpMultiply &op);
  void execute_op_divide(const bytecode::OpDivide &op);
  void execute_op_modulo(const bytecode::OpModulo &op);
  void execute_op_negate(const bytecode::OpNegate &op);
  void execute_op_not(const bytecode::OpNot &op);
  void execute_op_equal(const bytecode::OpEqual &op);
  void execute_op_not_equal(const bytecode::OpNotEqual &op);
  void execute_op_greater(const bytecode::OpGreater &op);
  void execute_op_greater_equal(const bytecode::OpGreaterEqual &op);
  void execute_op_less(const bytecode::OpLess &op);
  void execute_op_less_equal(const bytecode::OpLessEqual &op);
  void execute_op_jump(const bytecode::OpJump &op);
  void execute_op_jump_if_false(const bytecode::OpTest &op);
  void execute_op_get_global(
      const bytecode::OpGetGlobal &op, const bytecode::Chunk &chunk
  );
  void execute_op_set_global(
      const bytecode::OpSetGlobal &op, const bytecode::Chunk &chunk
  );
  void execute_op_get_local(const bytecode::OpGetLocal &op, CallFrame &frame);
  void execute_op_set_local(const bytecode::OpSetLocal &op, CallFrame &frame);
  void
  execute_op_mutate_local(const bytecode::OpMutateLocal &op, CallFrame &frame);
  void execute_op_make_array(const bytecode::OpMakeArray &op);
  void execute_op_get_index(const bytecode::OpGetIndex &op);
  void execute_op_set_index(const bytecode::OpSetIndex &op);
  void execute_op_call(const bytecode::OpCall &op);
  void execute_op_closure(
      const bytecode::OpClosure &op,
      const bytecode::Chunk &chunk,
      CallFrame &frame
  );
  void
  execute_op_get_upvalue(const bytecode::OpGetUpvalue &op, CallFrame &frame);
  void
  execute_op_set_upvalue(const bytecode::OpSetUpvalue &op, CallFrame &frame);

  runtime::Ref stack_pop();
  void stack_push(runtime::Value &&value);

  template <typename... Args>
  void debug_print(std::format_string<Args...> fmt, Args &&...args);

  template <typename Op> void binary_arithmetic(Op op);

  template <typename Predicate> void comparison_op(Predicate predicate);

  bool debug;
  runtime::GCStorage gc_storage;
  std::vector<runtime::Ref> stack;
  std::vector<runtime::Ref> file_scope_slots;
  std::map<std::string, std::size_t> file_scope_indices;

  std::vector<CallFrame> frames;
  const std::vector<bytecode::Chunk> *current_chunks = nullptr;
};

} // namespace l3::vm
