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

  runtime::StackValue store_value(runtime::Value &&value);

  template <typename T>
    requires std::constructible_from<runtime::Value, T &&>
  runtime::StackValue store_value(T &&value) {
    return store_value(runtime::Value{std::forward<T>(value)});
  }

  runtime::StackValue call_function(
      const runtime::StackValue &function,
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
    std::optional<std::pair<runtime::BytecodeFunction, runtime::StackValue>>
        closure;
    std::vector<runtime::GCUpvalue *> upvalues;
    std::unordered_map<std::size_t, runtime::GCUpvalue *> captured_locals;
  };

  void execute(bytecode::ProgramBytecode &program);

private:
  std::optional<runtime::StackValue>
  resolve_global(std::string_view name) const;
  void define_global(std::string_view name, runtime::StackValue value);

  [[nodiscard]] auto &&current_frame(this auto &&self);
  [[nodiscard]] std::size_t current_frame_pointer() const;
  [[nodiscard]] std::size_t frame_absolute_slot(std::size_t offset) const;

  [[nodiscard]] auto &&constant_at(this auto &&self, std::size_t index);
  [[nodiscard]] const location::Location &current_instruction_location() const;

  [[nodiscard]] auto &&stack_at(this auto &&self, std::size_t index);
  [[nodiscard]] auto &&stack_local(this auto &&self, std::size_t offset);
  [[nodiscard]] auto &&stack_top(this auto &&self, std::size_t offset = 0);

  void execute_loop(std::size_t target_frames);

  void execute_op(const bytecode::OpReturn &op, CallFrame &);
  void execute_op(const bytecode::OpConstant &op, CallFrame &);
  void execute_op(const bytecode::OpPop &op, CallFrame &);
  void execute_op(const bytecode::OpDuplicate &op, CallFrame &);
  void execute_op(const bytecode::OpAdd &op, CallFrame &);
  void execute_op(const bytecode::OpSubtract &op, CallFrame &);
  void execute_op(const bytecode::OpMultiply &op, CallFrame &);
  void execute_op(const bytecode::OpDivide &op, CallFrame &);
  void execute_op(const bytecode::OpModulo &op, CallFrame &);
  void execute_op(const bytecode::OpPower &op, CallFrame &);
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

  runtime::StackValue stack_pop();
  void stack_push(runtime::StackValue value);

  template <typename... Args>
  void debug_print(std::format_string<Args...> fmt, Args &&...args);

  bool debug;
  runtime::GCStorage gc_storage;
  runtime::GCUpvalueStorage upvalue_storage;
  std::vector<runtime::StackValue> stack;
  std::unordered_map<
      std::string,
      runtime::StackValue,
      string_hash,
      std::equal_to<>>
      global_symbols;

  std::vector<CallFrame> frames;
  bytecode::ProgramBytecode *current_program = nullptr;
};

} // namespace l3::vm
