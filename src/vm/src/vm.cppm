export module l3.vm;

import std;
import l3.bytecode;
import l3.location;
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
  runtime::Value evaluate(
      const runtime::Value &function,
      runtime::L3Args arguments,
      const location::Location &call_location = {}
  );

  std::size_t run_gc();
  void maybe_gc();

  struct CallFrame {
    std::size_t chunk_id = 0;
    std::size_t ip = 0;
    std::size_t frame_pointer = 0;
    std::optional<location::Location> call_location;
    std::optional<std::pair<runtime::BytecodeFunction, runtime::Value>> closure;
  };

  void execute(bytecode::ProgramBytecode &program);

private:
  std::optional<runtime::Ref> resolve_global(std::string_view name) const;
  void define_global(std::string_view name, runtime::Ref value);

  [[nodiscard]] auto &&current_frame(this auto &&self);
  [[nodiscard]] std::size_t current_frame_pointer() const;
  [[nodiscard]] std::size_t frame_absolute_slot(std::size_t offset) const;

  [[nodiscard]] auto &&constant_at(this auto &&self, std::size_t index);
  [[nodiscard]] const location::Location &current_instruction_location() const;

  [[nodiscard]] auto &&stack_at(this auto &&self, std::size_t index);
  [[nodiscard]] auto &&stack_local(this auto &&self, std::size_t offset);
  [[nodiscard]] auto &&stack_top(this auto &&self, std::size_t offset = 0);

  void execute_loop(std::size_t target_frames);

  template <typename T> void execute_op(const T &op, CallFrame &frame);

  void execute_op(const bytecode::OpReturn &op, CallFrame &);
  void execute_op(const bytecode::OpConstant &op, CallFrame &);
  void execute_op(const bytecode::OpPop &op, CallFrame &);
  void execute_op(const bytecode::OpDuplicate &op, CallFrame &);
  void execute_op(const bytecode::OpAdd &op, CallFrame &);
  void execute_op(const bytecode::OpSubtract &op, CallFrame &);
  void execute_op(const bytecode::OpMultiply &op, CallFrame &);
  void execute_op(const bytecode::OpDivide &op, CallFrame &);
  void execute_op(const bytecode::OpModulo &op, CallFrame &);
  void execute_op(const bytecode::OpNegate &op, CallFrame &);
  void execute_op(const bytecode::OpNot &op, CallFrame &);
  void execute_op(const bytecode::OpEqual &op, CallFrame &);
  void execute_op(const bytecode::OpNotEqual &op, CallFrame &);
  void execute_op(const bytecode::OpGreater &op, CallFrame &);
  void execute_op(const bytecode::OpGreaterEqual &op, CallFrame &);
  void execute_op(const bytecode::OpLess &op, CallFrame &);
  void execute_op(const bytecode::OpLessEqual &op, CallFrame &);
  void execute_op(const bytecode::OpJump &op, CallFrame &);
  void execute_op(const bytecode::OpJumpIf &op, CallFrame &);
  void execute_op(const bytecode::OpGetGlobal &op, CallFrame &);
  void execute_op(const bytecode::OpSetGlobal &op, CallFrame &);
  void execute_op(const bytecode::OpGetLocal &op, CallFrame &frame);
  void execute_op(const bytecode::OpSetLocal &op, CallFrame &frame);
  void execute_op(const bytecode::OpForLoop &op, CallFrame &frame);
  void execute_op(const bytecode::OpMakeArray &op, CallFrame &);
  void execute_op(const bytecode::OpGetIndex &op, CallFrame &);
  void execute_op(const bytecode::OpSetIndex &op, CallFrame &);
  void execute_op(const bytecode::OpCall &op, CallFrame &);
  void execute_op(const bytecode::OpClosure &op, CallFrame &frame);
  void execute_op(const bytecode::OpGetUpvalue &op, CallFrame &frame);
  void execute_op(const bytecode::OpSetUpvalue &op, CallFrame &frame);

  runtime::Value stack_pop();
  void stack_push(const runtime::Value &value);
  void stack_push(runtime::Value &&value);
  void stack_push_new(runtime::NewValue &&value);

  template <typename... Args>
  void debug_print(std::format_string<Args...> fmt, Args &&...args);

  template <typename Op> void binary_arithmetic(Op op);

  template <typename Predicate> void comparison_op(Predicate predicate);

  bool debug;
  runtime::GCStorage gc_storage;
  std::vector<runtime::Value> stack;
  std::unordered_map<std::string, runtime::Ref, string_hash, std::equal_to<>>
      global_symbols;

  std::vector<CallFrame> frames;
  bytecode::ProgramBytecode *current_program = nullptr;
};

} // namespace l3::vm
