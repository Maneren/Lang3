module l3.vm;

import std;
import l3.bytecode;
import l3.runtime;
import utils;

namespace l3::vm {

namespace {

std::string function_name_for_frame(const BytecodeVM::CallFrame &frame) {
  if (!frame.closure) {
    return "<toplevel>";
  }

  return frame.closure->first.name;
}

} // namespace

BytecodeVM::BytecodeVM(bool debug_) : debug(debug_) {
  for (const auto &[name, body] : l3::builtins::BUILTINS) {
    auto func = store_value(
        runtime::Function{runtime::BuiltinFunction{
            runtime::Identifier{std::string(name)},
            [this, body](runtime::L3Args args) -> runtime::Value {
              return body(*this, args);
            }
        }}
    );
    define_global(name, func);
  }
}

runtime::Ref BytecodeVM::store_value(runtime::Value &&value) {
  return runtime::Ref{gc_storage.emplace(std::move(value))};
}

runtime::Ref BytecodeVM::store_new_value(runtime::NewValue &&value) {
  return match::match(
      std::move(value),
      [](runtime::Ref ref) -> runtime::Ref { return ref; },
      [this](runtime::Value &&value) -> runtime::Ref {
        return runtime::Ref{gc_storage.emplace(std::move(value))};
      }
  );
}

auto &&BytecodeVM::current_frame(this auto &&self) {
  return self.frames.back();
}

const location::Location &BytecodeVM::current_instruction_location() const {
  return current_program->chunks[current_frame().chunk_id]
      .locations[current_frame().ip - 1];
}

runtime::Value BytecodeVM::stack_pop() {
  auto val = std::move(stack.back());
  stack.pop_back();
  return val;
}

void BytecodeVM::stack_push(runtime::Value &&value) {
  stack.emplace_back(std::move(value));
}

void BytecodeVM::stack_push(const runtime::Value &value) {
  stack.push_back(value);
}

void BytecodeVM::stack_push_new(runtime::NewValue &&value) {
  match::match(
      std::move(value),
      [this](runtime::Ref ref) { stack.emplace_back(*ref); },
      [this](runtime::Value &&value) { stack.emplace_back(std::move(value)); }
  );
}

std::optional<runtime::Ref>
BytecodeVM::resolve_global(std::string_view name) const {
  if (auto it = global_symbols.find(name); it != global_symbols.end()) {
    return it->second;
  }
  return std::nullopt;
}

void BytecodeVM::define_global(std::string_view name, runtime::Ref value) {
  if (auto it = global_symbols.find(name); it != global_symbols.end()) {
    throw runtime::RuntimeError("Global already defined: {}", name);
  }

  global_symbols.emplace(name, value);
}

std::size_t BytecodeVM::current_frame_pointer() const {
  return current_frame().frame_pointer;
}

std::size_t BytecodeVM::frame_absolute_slot(std::size_t offset) const {
  return current_frame_pointer() + offset;
}

auto &&BytecodeVM::constant_at(this auto &&self, std::size_t index) {
  return self.current_program->constants[index];
}

auto &&BytecodeVM::stack_at(this auto &&self, std::size_t index) {
  return self.stack[index];
}

auto &&BytecodeVM::stack_local(this auto &&self, std::size_t offset) {
  return self.stack_at(self.frame_absolute_slot(offset));
}

auto &&BytecodeVM::stack_top(this auto &&self, std::size_t offset) {
  return self.stack_at(self.stack.size() - offset - 1);
}

template <typename... Args>
void BytecodeVM::debug_print(std::format_string<Args...> fmt, Args &&...args) {
  if (debug) [[unlikely]] {
    std::println(std::cerr, fmt, std::forward<Args>(args)...);
  }
}

template <typename Op> void BytecodeVM::binary_arithmetic(Op op) {
  auto b = stack_pop();
  auto a = stack_pop();
  stack_push(op(a, b));
}

template <typename Predicate>
void BytecodeVM::comparison_op(Predicate predicate) {
  auto b = stack_pop();
  auto a = stack_pop();
  stack_push(runtime::Primitive{predicate(a.compare(b))});
}

runtime::Value BytecodeVM::evaluate(
    const runtime::Value &function,
    runtime::L3Args arguments,
    const location::Location &call_location
) {
  if (auto func_opt = function.as_function()) {
    const auto &func = func_opt->get();

    return func->visit(
        [&](const runtime::BuiltinFunction &func) {
          return func.invoke(arguments);
        },
        [&](const runtime::BytecodeFunction &bc_func) {
          auto total_args = bc_func.curried_args.size() + arguments.size();

          if (total_args < bc_func.arity) {
            auto new_func = bc_func;
            for (const auto &arg : arguments) {
              new_func.curried_args.push_back(store_value(runtime::Value{arg}));
            }
            return runtime::Value{runtime::Function{std::move(new_func)}};
          }
          if (total_args == bc_func.arity) {
            auto previous_frames = frames.size();
            auto fp = stack.size();
            frames.emplace_back(
                bc_func.id,
                0,
                fp,
                call_location,
                std::pair{bc_func, runtime::Value{function}}
            );
            for (const auto &arg : bc_func.curried_args) {
              stack.push_back(*arg);
            }
            for (const auto &arg : arguments) {
              stack.push_back(arg);
            }
            execute_loop(previous_frames);
            return stack_pop();
          }

          throw runtime::RuntimeError("evaluate arity mismatch");
        }
    );
  }
  throw runtime::RuntimeError("evaluate: not a function");
}

std::size_t BytecodeVM::run_gc() {
  for (auto &ref : stack) {
    if (auto vec_opt = ref.as_vector()) {
      const auto &vec = vec_opt->get();
      if (vec) {
        for (auto &item : *vec) {
          const_cast<runtime::Ref &>(item).get_gc_mut().mark();
        }
      }
    } else if (auto func_opt = ref.as_function()) {
      func_opt->get()->visit(
          [](const runtime::BuiltinFunction &) {},
          [](const runtime::BytecodeFunction &bc_func) {
            for (auto &arg : const_cast<std::vector<runtime::Ref> &>(
                     bc_func.curried_args
                 )) {
              arg.get_gc_mut().mark();
            }
          }
      );
    }
  }
  for (auto &[_, ref] : global_symbols) {
    const_cast<runtime::Ref &>(ref).get_gc_mut().mark();
  }
  for (auto &frame : frames) {
    if (frame.closure) {
      if (auto vec_opt = frame.closure->second.as_vector()) {
        const auto &vec = vec_opt->get();
        if (vec) {
          for (auto &ref : *vec) {
            const_cast<runtime::Ref &>(ref).get_gc_mut().mark();
          }
        }
      } else if (auto func_opt = frame.closure->second.as_function()) {
        func_opt->get()->visit(
            [](const runtime::BuiltinFunction &) {},
            [](const runtime::BytecodeFunction &bc_func) {
              for (auto &arg : const_cast<std::vector<runtime::Ref> &>(
                       bc_func.curried_args
                   )) {
                arg.get_gc_mut().mark();
              }
            }
        );
      }
    }
  }
  return gc_storage.sweep();
}

void BytecodeVM::maybe_gc() {
  constexpr std::size_t GC_INTERVAL = 16UZ * 1024UZ;
  if (gc_storage.get_added_since_last_sweep() > GC_INTERVAL) {
    run_gc();
  }
}

[[clang::noinline]]
void BytecodeVM::execute(bytecode::ProgramBytecode &program) {
  current_program = &program;
  frames.emplace_back();
  try {
    execute_loop(0);
  } catch (runtime::RuntimeError &error) {
    if (!error.get_location()) {
      error.set_location(current_instruction_location());
    }
    auto stacktrace =
        std::views::filter(
            frames,
            [](const auto &frame) { return frame.call_location.has_value(); }
        ) |
        std::views::transform([](const auto &frame) {
          return runtime::StacktraceFrame{
              .function_name = function_name_for_frame(frame),
              .call_location = *frame.call_location
          };
        }) |
        std::ranges::to<std::vector>();
    error.set_stacktrace(std::move(stacktrace));
    current_program = nullptr;
    throw;
  }
  current_program = nullptr;

  if (stack.size() != 1 || stack.back().compare(runtime::Value{}) != 0) {
    std::println(std::cerr, "{}", stack);
    throw std::runtime_error("stack not empty after program execution");
  }
}

[[clang::noinline]]
void BytecodeVM::execute_loop(std::size_t target_frames) {
  const auto &program = *current_program;
  const auto &chunks = program.chunks;

  while (frames.size() > target_frames) {
    maybe_gc();
    auto &frame = frames.back();
    const auto &chunk = chunks[frame.chunk_id];

    debug_print("IP: {}", frame.ip);

    const auto &instruction = chunk.code[frame.ip++];

    match::match(instruction, [&, this](const auto &op) {
      execute_op(op, frame);
    });
  }
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpReturn & /*op*/, CallFrame & /*frame*/) {
  auto result = stack_pop();

  debug_print("RETURN value={}", result);

  auto fp = frames.back().frame_pointer;
  frames.pop_back();
  stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(fp), stack.end());

  stack.push_back(std::move(result));
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpConstant &op, CallFrame & /*frame*/) {
  const runtime::GCValue &chunk_val = constant_at(op.index);
  stack.push_back(chunk_val.get_value());
  debug_print("CONSTANT index={} value={}", op.index, stack_top());
}

[[clang::noinline]]
void BytecodeVM::execute_op(const bytecode::OpPop &op, CallFrame & /*frame*/) {
  if (stack.empty() || stack.size() == current_frame_pointer()) {
    throw runtime::RuntimeError("stack underflow");
  }

  if (op.count == 1) {
    debug_print("POP value={}", stack_pop());
  } else {
    auto base = stack.end() - static_cast<std::ptrdiff_t>(op.count);
    debug_print("POP values={}", 0); // Temporary fix
    stack.erase(base, stack.end());
  }
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpDuplicate &op, CallFrame & /*frame*/) {
  debug_print("DUPLICATE value={}", stack_top(op.index));
  stack.push_back(stack_top(op.index));
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpAdd & /*op*/, CallFrame & /*frame*/) {
  debug_print("ADD a={} b={}", stack_top(1), stack_top());
  binary_arithmetic([](auto &a, auto &b) { return a.add(b); });
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpSubtract & /*op*/, CallFrame & /*frame*/) {
  debug_print("SUBTRACT a={} b={}", stack_top(1), stack_top());
  binary_arithmetic([](auto &a, auto &b) { return a.sub(b); });
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpMultiply & /*op*/, CallFrame & /*frame*/) {
  debug_print("MULTIPLY a={} b={}", stack_top(1), stack_top());
  binary_arithmetic([](auto &a, auto &b) { return a.mul(b); });
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpDivide & /*op*/, CallFrame & /*frame*/) {
  debug_print("DIVIDE a={} b={}", stack_top(1), stack_top());
  binary_arithmetic([](auto &a, auto &b) { return a.div(b); });
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpModulo & /*op*/, CallFrame & /*frame*/) {
  debug_print("MODULO a={} b={}", stack_top(1), stack_top());
  binary_arithmetic([](auto &a, auto &b) { return a.mod(b); });
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpNegate & /*op*/, CallFrame & /*frame*/) {
  debug_print("NEGATE a={}", stack_top());
  auto a = stack_pop();
  stack_push(a.negative());
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpNot & /*op*/, CallFrame & /*frame*/) {
  debug_print("NOT a={}", stack_top());
  auto a = stack_pop();
  stack_push(a.not_op());
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpEqual & /*op*/, CallFrame & /*frame*/) {
  debug_print("EQUAL a={} b={}", stack_top(1), stack_top());
  comparison_op([](auto c) { return c == std::partial_ordering::equivalent; });
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpNotEqual & /*op*/, CallFrame & /*frame*/) {
  debug_print("NOT_EQUAL a={} b={}", stack_top(1), stack_top());
  comparison_op([](auto c) { return c != std::partial_ordering::equivalent; });
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpGreater & /*op*/, CallFrame & /*frame*/) {
  debug_print("GREATER a={} b={}", stack_top(1), stack_top());
  comparison_op([](auto c) { return c == std::partial_ordering::greater; });
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpGreaterEqual & /*op*/, CallFrame & /*frame*/) {
  debug_print("GREATER_EQUAL a={} b={}", stack_top(1), stack_top());
  comparison_op([](auto c) {
    return c == std::partial_ordering::greater ||
           c == std::partial_ordering::equivalent;
  });
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpLess & /*op*/, CallFrame & /*frame*/) {
  debug_print("LESS a={} b={}", stack_top(1), stack_top());
  comparison_op([](auto c) { return c == std::partial_ordering::less; });
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpLessEqual & /*op*/, CallFrame & /*frame*/) {
  debug_print("LESS_EQUAL a={} b={}", stack_top(1), stack_top());
  comparison_op([](auto c) {
    return c == std::partial_ordering::less ||
           c == std::partial_ordering::equivalent;
  });
}

[[clang::noinline]]
void BytecodeVM::execute_op(const bytecode::OpJump &op, CallFrame & /*frame*/) {
  debug_print("JUMP target={}", op.offset);
  current_frame().ip = op.offset;
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpJumpIf &op, CallFrame & /*frame*/) {
  const bool jump = stack_top().is_truthy() == op.expected;
  const auto pop = jump ? !op.keep_jump : !op.keep_stay;

  debug_print(
      "JUMP_IF condition={} == {} target={} pop={}",
      stack_top(),
      op.expected,
      op.offset,
      pop
  );

  if (jump) {
    current_frame().ip = op.offset;
  }

  if (pop) {
    stack_pop();
  }
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpGetGlobal &op, CallFrame & /*frame*/) {
  const auto &name_val = constant_at(op.name_index);
  if (auto str_opt = name_val.get_value().as_string()) {
    const auto slot = resolve_global(str_opt->get());
    if (!slot) {
      throw runtime::UndefinedVariableError("{}", str_opt->get());
    }
    const auto &value = *slot;
    debug_print("GET_GLOBAL name={} value={}", str_opt->get(), value);
    stack.push_back(value.get());
  }
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpSetGlobal &op, CallFrame & /*frame*/) {
  const auto &name_val = constant_at(op.name_index);
  if (auto str_opt = name_val.get_value().as_string()) {
    auto slot = resolve_global(str_opt->get());
    if (!slot) {
      throw runtime::RuntimeError("Undefined variable: {}", str_opt->get());
    }
    debug_print("SET_GLOBAL name={} value={}", str_opt->get(), stack_top());
    *slot = store_value(stack_pop());
  }
}

[[clang::noinline]]
void BytecodeVM::execute_op(const bytecode::OpGetLocal &op, CallFrame &frame) {
  stack.push_back(stack_at(frame.frame_pointer + op.index));
  debug_print(
      "GET_LOCAL index={} fp={} stack size={} value={}",
      op.index,
      frame.frame_pointer,
      stack.size(),
      stack.back()
  );
}

[[clang::noinline]]
void BytecodeVM::execute_op(const bytecode::OpSetLocal &op, CallFrame &frame) {
  debug_print("SET_LOCAL index={} value={}", op.index, stack_top());
  stack_at(frame.frame_pointer + op.index) = stack_pop();
}

[[clang::noinline]]
void BytecodeVM::execute_op(const bytecode::OpForLoop &op, CallFrame &frame) {
  const auto control_slot = frame.frame_pointer + op.control_index;
  const auto limit_slot = frame.frame_pointer + op.limit_index;

  if (!stack_at(control_slot).is_primitive() ||
      !stack_at(control_slot).as_primitive()->get().is_integer()) {
    throw runtime::RuntimeError("for-loop control variable must be an integer");
  }
  if (!stack_at(limit_slot).is_primitive() ||
      !stack_at(limit_slot).as_primitive()->get().is_integer()) {
    throw runtime::RuntimeError("for-loop limit value must be an integer");
  }

  std::int64_t step = 1;
  if (op.step_index) {
    const auto step_slot = frame.frame_pointer + *op.step_index;
    if (!stack_at(step_slot).is_primitive() ||
        !stack_at(step_slot).as_primitive()->get().is_integer()) {
      throw runtime::RuntimeError("for-loop step value must be an integer");
    }
    step = stack_at(step_slot).as_primitive()->get().as_integer().value();
  }

  const auto current =
      stack_at(control_slot).as_primitive()->get().as_integer().value();
  const auto next = current + step;
  stack_at(control_slot) =
      runtime::Value{runtime::Primitive{static_cast<long>(next)}};

  const auto limit =
      stack_at(limit_slot).as_primitive()->get().as_integer().value();

  const bool keep_running = op.inclusive
                                ? (step >= 0 ? next <= limit : next >= limit)
                                : (step >= 0 ? next < limit : next > limit);

  if (keep_running) {
    current_frame().ip = op.body_offset;
  }

  debug_print(
      "FOR_LOOP ctrl={} lim={} step={} next={} body={} take={}",
      op.control_index,
      op.limit_index,
      step,
      next,
      op.body_offset,
      keep_running
  );
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpMakeArray &op, CallFrame & /*frame*/) {
  const auto start = stack.end() - static_cast<std::ptrdiff_t>(op.count);
  auto elements = std::make_shared<std::vector<runtime::Ref>>();
  elements->reserve(op.count);
  for (auto it = start; it != stack.end(); ++it) {
    elements->push_back(store_value(std::move(*it)));
  }
  stack.erase(start, stack.end());
  debug_print("MAKE_ARRAY count={}", op.count);
  stack_push(runtime::Value{std::move(elements)});
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpGetIndex & /*op*/, CallFrame & /*frame*/) {
  const auto index = stack_pop();
  const auto array = stack_pop();

  debug_print("GET_INDEX array={} index={}", array, index);
  stack_push_new(array.index(index));
}

[[clang::noinline]]
void BytecodeVM::
    execute_op(const bytecode::OpSetIndex & /*op*/, CallFrame & /*frame*/) {
  auto value = stack_pop();
  auto index = stack_pop();

  auto &array = stack.back();
  debug_print("SET_INDEX array={} index={} value={}", array, index, value);

  array.index_mut(index) = store_value(std::move(value));
  stack.pop_back();
}

[[clang::noinline]]
void BytecodeVM::execute_op(const bytecode::OpCall &op, CallFrame & /*frame*/) {
  auto args_base_ptr = stack.end() - static_cast<std::ptrdiff_t>(op.arg_count);
  std::vector<runtime::Value> args;
  args.reserve(op.arg_count);
  for (auto it = args_base_ptr; it != stack.end(); ++it) {
    args.push_back(*it);
  }
  stack.erase(args_base_ptr, stack.end());

  auto func_ref = stack_pop();
  debug_print("CALL func={} args={}", func_ref, args);

  if (!func_ref.is_function()) {
    throw runtime::RuntimeError("Attempted to call a non-function value");
  }

  auto result =
      evaluate(func_ref, std::move(args), current_instruction_location());
  if (op.keep_return_value) {
    stack.push_back(std::move(result));
  }
}

[[clang::noinline]]
void BytecodeVM::execute_op(const bytecode::OpClosure &op, CallFrame &frame) {
  auto function = current_program->constants[op.function_index]
                      .get_value_mut()
                      .as_mut_function()
                      ->get()
                      ->as_mut_bytecode_function()
                      ->get();

  for (const auto &[local, index] : op.upvalues) {
    const auto upvalue = local ? frame.frame_pointer + index
                               : frame.closure->first.upvalues[index];

    function.upvalues.emplace_back(upvalue);
  }
  stack_push(runtime::Function{std::move(function)});
  debug_print("CLOSURE function={}", stack.back());
}

[[clang::noinline]]
void BytecodeVM::execute_op(
    const bytecode::OpGetUpvalue &op, CallFrame &frame
) {
  const auto &upvalues = frame.closure->first.upvalues;
  debug_print("upvalues: {}", upvalues);
  const auto ptr = upvalues[op.index];
  const auto &val = stack[ptr];
  debug_print("GET_UPVALUE index={} ptr={} value={}", op.index, ptr, val);
  stack.push_back(val);
}

[[clang::noinline]]
void BytecodeVM::execute_op(
    const bytecode::OpSetUpvalue &op, CallFrame &frame
) {
  debug_print("SET_UPVALUE index={}", op.index, stack_top());
  frame.closure->first.upvalues[op.index] = stack.size() - 1;
}

} // namespace l3::vm
