export module l3.vm;

import std;
import l3.bytecode;
import l3.runtime;
import utils;
import :builtins;

namespace {
struct string_hash {

  using is_transparent = void;
  [[nodiscard]] std::size_t operator()(const char *txt) const {
    return std::hash<std::string_view>{}(txt);
  }
  [[nodiscard]] std::size_t operator()(std::string_view txt) const {
    return std::hash<std::string_view>{}(txt);
  }
  [[nodiscard]] std::size_t operator()(const std::string &txt) const {
    return std::hash<std::string>{}(txt);
  }
};

} // namespace

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

  void execute(bytecode::ProgramBytecode &program);

private:
  std::optional<runtime::Ref> resolve_global(std::string_view name) const;
  void define_global(std::string_view name, runtime::Ref value);

  void assign_slot(runtime::Ref &slot);

  void execute_loop(std::size_t target_frames);

  void execute_op_return(const bytecode::OpReturn &op);
  void execute_op_constant(const bytecode::OpConstant &op);
  void execute_op_pop(const bytecode::OpPop &op);
  void execute_op_duplicate(const bytecode::OpDuplicate &op);
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
  void execute_op_get_global(const bytecode::OpGetGlobal &op);
  void execute_op_set_global(const bytecode::OpSetGlobal &op);
  void execute_op_get_local(const bytecode::OpGetLocal &op, CallFrame &frame);
  void execute_op_set_local(const bytecode::OpSetLocal &op, CallFrame &frame);
  void execute_op_for_loop(const bytecode::OpForLoop &op, CallFrame &frame);
  void execute_op_make_array(const bytecode::OpMakeArray &op);
  void execute_op_get_index(const bytecode::OpGetIndex &op);
  void execute_op_set_index(const bytecode::OpSetIndex &op);
  void execute_op_call(const bytecode::OpCall &op);
  void execute_op_closure(const bytecode::OpClosure &op, CallFrame &frame);
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
  std::unordered_map<std::string, runtime::Ref, string_hash, std::equal_to<>>
      global_symbols;

  std::vector<CallFrame> frames;
  bytecode::ProgramBytecode *current_program = nullptr;
};

} // namespace l3::vm
