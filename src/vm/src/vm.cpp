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

  const auto &function_value = frame.closure->get();
  if (const auto function = function_value.as_function()) {
    if (const auto bytecode = function->get()->as_bytecode_function()) {
      return bytecode->get().name;
    }

    if (const auto builtin = function->get()->as_builtin_function()) {
      return builtin->get().get_name().get_name();
    }
  }

  return "<function>";
}

} // namespace

BytecodeVM::BytecodeVM(bool debug_) : debug(debug_) {
  for (const auto &[name, body] : l3::builtins::BUILTINS) {
    auto func = store_value(
        runtime::Function{runtime::BuiltinFunction{
            runtime::Identifier{std::string(name)},
            [this, &body](runtime::L3Args args) { return body(*this, args); }
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

const location::Location &BytecodeVM::current_instruction_location() const {
  return current_program->chunks[current_frame().chunk_id]
      .locations[current_frame().ip - 1];
}

runtime::Ref BytecodeVM::stack_pop() {
  auto val = stack.back();
  stack.pop_back();
  return val;
}

void BytecodeVM::stack_push(runtime::Value &&value) {
  stack.emplace_back(store_value(std::move(value)));
}

void BytecodeVM::stack_push_new(runtime::NewValue &&value) {
  stack.emplace_back(store_new_value(std::move(value)));
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

void BytecodeVM::assign_slot(runtime::Ref &slot) { slot = stack_pop(); }

BytecodeVM::CallFrame &BytecodeVM::current_frame() { return frames.back(); }

const BytecodeVM::CallFrame &BytecodeVM::current_frame() const {
  return frames.back();
}

std::size_t BytecodeVM::current_frame_pointer() const {
  return current_frame().frame_pointer;
}

std::size_t BytecodeVM::frame_absolute_slot(std::size_t offset) const {
  return current_frame_pointer() + offset;
}

runtime::GCValue &BytecodeVM::constant_at(std::size_t index) {
  return current_program->constants[index];
}

const runtime::GCValue &BytecodeVM::constant_at(std::size_t index) const {
  return current_program->constants[index];
}

runtime::Ref &BytecodeVM::stack_at(std::size_t index) { return stack[index]; }

const runtime::Ref &BytecodeVM::stack_at(std::size_t index) const {
  return stack[index];
}

runtime::Ref &BytecodeVM::stack_local(std::size_t offset) {
  return stack_at(frame_absolute_slot(offset));
}

const runtime::Ref &BytecodeVM::stack_local(std::size_t offset) const {
  return stack_at(frame_absolute_slot(offset));
}

runtime::Ref &BytecodeVM::stack_top(std::size_t offset) {
  return stack_at(stack.size() - offset - 1);
}

const runtime::Ref &BytecodeVM::stack_top(std::size_t offset) const {
  return stack_at(stack.size() - offset - 1);
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
  stack_push(op(*a, *b));
}

template <typename Predicate>
void BytecodeVM::comparison_op(Predicate predicate) {
  auto b = stack_pop();
  auto a = stack_pop();
  stack_push(runtime::Primitive{predicate(a->compare(*b))});
}

runtime::Ref BytecodeVM::evaluate(
    runtime::Ref function,
    runtime::L3Args arguments,
    const location::Location &call_location
) {
  if (auto func_opt = function->as_function()) {
    const auto &func = func_opt->get();

    return func->visit(
        [&](const runtime::BuiltinFunction &func) {
          return func.invoke(arguments);
        },
        [&](const runtime::BytecodeFunctionId &bc_func) {
          auto total_args = bc_func.curried_args.size() + arguments.size();

          if (total_args < bc_func.arity) {
            auto new_func = bc_func;
            new_func.curried_args.append_range(arguments);
            return store_value(
                runtime::Value{runtime::Function{std::move(new_func)}}
            );
          }
          if (total_args == bc_func.arity) {
            auto previous_frames = frames.size();
            auto fp = stack.size();
            frames.emplace_back(
                bc_func.id, 0, fp, call_location, std::make_optional(function)
            );
            for (const auto &arg : bc_func.curried_args) {
              stack.push_back(arg);
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
    ref.get_gc_mut().mark();
  }
  for (auto &[_, ref] : global_symbols) {
    ref.get_gc_mut().mark();
  }
  for (auto &frame : frames) {
    if (frame.closure) {
      frame.closure->get_gc_mut().mark();
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

  if (stack.size() != 1 || stack.back()->compare(runtime::Value{}) != 0) {
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

    match::match(
        instruction,
        [&](const bytecode::OpReturn &op) { execute_op_return(op); },
        [&](const bytecode::OpConstant &op) { execute_op_constant(op); },
        [&](const bytecode::OpPop &op) { execute_op_pop(op); },
        [&](const bytecode::OpDuplicate &op) { execute_op_duplicate(op); },
        [&](const bytecode::OpAdd &op) { execute_op_add(op); },
        [&](const bytecode::OpSubtract &op) { execute_op_subtract(op); },
        [&](const bytecode::OpMultiply &op) { execute_op_multiply(op); },
        [&](const bytecode::OpDivide &op) { execute_op_divide(op); },
        [&](const bytecode::OpModulo &op) { execute_op_modulo(op); },
        [&](const bytecode::OpNegate &op) { execute_op_negate(op); },
        [&](const bytecode::OpNot &op) { execute_op_not(op); },
        [&](const bytecode::OpEqual &op) { execute_op_equal(op); },
        [&](const bytecode::OpNotEqual &op) { execute_op_not_equal(op); },
        [&](const bytecode::OpGreater &op) { execute_op_greater(op); },
        [&](const bytecode::OpGreaterEqual &op) {
          execute_op_greater_equal(op);
        },
        [&](const bytecode::OpLess &op) { execute_op_less(op); },
        [&](const bytecode::OpLessEqual &op) { execute_op_less_equal(op); },
        [&](const bytecode::OpJump &op) { execute_op_jump(op); },
        [&](const bytecode::OpJumpIf &op) { execute_op_jump_if(op); },
        [&](const bytecode::OpGetGlobal &op) { execute_op_get_global(op); },
        [&](const bytecode::OpSetGlobal &op) { execute_op_set_global(op); },
        [&](const bytecode::OpGetLocal &op) {
          execute_op_get_local(op, frame);
        },
        [&](const bytecode::OpSetLocal &op) {
          execute_op_set_local(op, frame);
        },
        [&](const bytecode::OpForLoop &op) { execute_op_for_loop(op, frame); },
        [&](const bytecode::OpMakeArray &op) { execute_op_make_array(op); },
        [&](const bytecode::OpGetIndex &op) { execute_op_get_index(op); },
        [&](const bytecode::OpSetIndex &op) { execute_op_set_index(op); },
        [&](const bytecode::OpCall &op) { execute_op_call(op); },
        [&](const bytecode::OpClosure &op) { execute_op_closure(op, frame); },
        [&](const bytecode::OpGetUpvalue &op) {
          execute_op_get_upvalue(op, frame);
        },
        [&](const bytecode::OpSetUpvalue &op) {
          execute_op_set_upvalue(op, frame);
        }
    );
  }
}

[[clang::noinline]]
void BytecodeVM::execute_op_return(const bytecode::OpReturn & /*op*/) {
  const auto result = stack_pop();

  debug_print("RETURN value={}", result);

  auto fp = frames.back().frame_pointer;
  frames.pop_back();
  stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(fp), stack.end());

  stack.push_back(result);
}

[[clang::noinline]]
void BytecodeVM::execute_op_constant(const bytecode::OpConstant &op) {
  runtime::GCValue &chunk_val = constant_at(op.index);
  stack.emplace_back(chunk_val);
  debug_print("CONSTANT index={} value={}", op.index, stack_top());
}

[[clang::noinline]]
void BytecodeVM::execute_op_pop(const bytecode::OpPop &op) {
  if (stack.empty() || stack.size() == current_frame_pointer()) {
    throw runtime::RuntimeError("stack underflow");
  }

  if (op.count == 1) {
    debug_print("POP value={}", stack_pop());
  } else {
    auto base = stack.end() - static_cast<std::ptrdiff_t>(op.count);
    debug_print("POP values={}", std::vector(base, stack.end()));
    stack.erase(base, stack.end());
  }
}

[[clang::noinline]]
void BytecodeVM::execute_op_duplicate(const bytecode::OpDuplicate &op) {
  const auto ref = stack_top(op.index);
  debug_print("DUPLICATE value={}", ref);
  stack.emplace_back(ref);
}

[[clang::noinline]]
void BytecodeVM::execute_op_add(const bytecode::OpAdd & /*op*/) {
  debug_print("ADD a={} b={}", stack_top(1), stack_top());
  binary_arithmetic([](auto &a, auto &b) { return a.add(b); });
}

[[clang::noinline]]
void BytecodeVM::execute_op_subtract(const bytecode::OpSubtract & /*op*/) {
  debug_print("SUBTRACT a={} b={}", stack_top(1), stack_top());
  binary_arithmetic([](auto &a, auto &b) { return a.sub(b); });
}

[[clang::noinline]]
void BytecodeVM::execute_op_multiply(const bytecode::OpMultiply & /*op*/) {
  debug_print("MULTIPLY a={} b={}", stack_top(1), stack_top());
  binary_arithmetic([](auto &a, auto &b) { return a.mul(b); });
}

[[clang::noinline]]
void BytecodeVM::execute_op_divide(const bytecode::OpDivide & /*op*/) {
  debug_print("DIVIDE a={} b={}", stack_top(1), stack_top());
  binary_arithmetic([](auto &a, auto &b) { return a.div(b); });
}

[[clang::noinline]]
void BytecodeVM::execute_op_modulo(const bytecode::OpModulo & /*op*/) {
  debug_print("MODULO a={} b={}", stack_top(1), stack_top());
  binary_arithmetic([](auto &a, auto &b) { return a.mod(b); });
}

[[clang::noinline]]
void BytecodeVM::execute_op_negate(const bytecode::OpNegate & /*op*/) {
  debug_print("NEGATE a={}", stack_top());
  auto a = stack_pop();
  stack_push(a->negative());
}

[[clang::noinline]]
void BytecodeVM::execute_op_not(const bytecode::OpNot & /*op*/) {
  debug_print("NOT a={}", stack_top());
  auto a = stack_pop();
  stack_push(a->not_op());
}

[[clang::noinline]]
void BytecodeVM::execute_op_equal(const bytecode::OpEqual & /*op*/) {
  debug_print("EQUAL a={} b={}", stack_top(1), stack_top());
  comparison_op([](auto c) { return c == std::partial_ordering::equivalent; });
}

[[clang::noinline]]
void BytecodeVM::execute_op_not_equal(const bytecode::OpNotEqual & /*op*/) {
  debug_print("NOT_EQUAL a={} b={}", stack_top(1), stack_top());
  comparison_op([](auto c) { return c != std::partial_ordering::equivalent; });
}

[[clang::noinline]]
void BytecodeVM::execute_op_greater(const bytecode::OpGreater & /*op*/) {
  debug_print("GREATER a={} b={}", stack_top(1), stack_top());
  comparison_op([](auto c) { return c == std::partial_ordering::greater; });
}

[[clang::noinline]]
void BytecodeVM::
    execute_op_greater_equal(const bytecode::OpGreaterEqual & /*op*/) {
  debug_print("GREATER_EQUAL a={} b={}", stack_top(1), stack_top());
  comparison_op([](auto c) {
    return c == std::partial_ordering::greater ||
           c == std::partial_ordering::equivalent;
  });
}

[[clang::noinline]]
void BytecodeVM::execute_op_less(const bytecode::OpLess & /*op*/) {
  debug_print("LESS a={} b={}", stack_top(1), stack_top());
  comparison_op([](auto c) { return c == std::partial_ordering::less; });
}

[[clang::noinline]]
void BytecodeVM::execute_op_less_equal(const bytecode::OpLessEqual & /*op*/) {
  debug_print("LESS_EQUAL a={} b={}", stack_top(1), stack_top());
  comparison_op([](auto c) {
    return c == std::partial_ordering::less ||
           c == std::partial_ordering::equivalent;
  });
}

[[clang::noinline]]
void BytecodeVM::execute_op_jump(const bytecode::OpJump &op) {
  debug_print("JUMP target={}", op.offset);
  current_frame().ip = op.offset;
}

[[clang::noinline]]
void BytecodeVM::execute_op_jump_if(const bytecode::OpJumpIf &op) {
  const bool jump = stack_top()->is_truthy() == op.expected;
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
void BytecodeVM::execute_op_get_global(const bytecode::OpGetGlobal &op) {
  const auto &name_val = constant_at(op.name_index);
  if (auto str_opt = name_val.get_value().as_string()) {
    const auto slot = resolve_global(str_opt->get());
    if (!slot) {
      throw runtime::UndefinedVariableError("{}", str_opt->get());
    }
    const auto &value = *slot;
    debug_print("GET_GLOBAL name={} value={}", str_opt->get(), value);
    stack.push_back(value);
  }
}

[[clang::noinline]]
void BytecodeVM::execute_op_set_global(const bytecode::OpSetGlobal &op) {
  const auto &name_val = constant_at(op.name_index);
  if (auto str_opt = name_val.get_value().as_string()) {
    auto slot = resolve_global(str_opt->get());
    if (!slot) {
      throw runtime::RuntimeError("Undefined variable: {}", str_opt->get());
    }
    debug_print("SET_GLOBAL name={} value={}", str_opt->get(), stack_top());
    assign_slot(*slot);
  }
}

[[clang::noinline]]
void BytecodeVM::execute_op_get_local(
    const bytecode::OpGetLocal &op, CallFrame &frame
) {
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
void BytecodeVM::execute_op_set_local(
    const bytecode::OpSetLocal &op, CallFrame &frame
) {
  debug_print("SET_LOCAL index={} value={}", op.index, stack_top());
  assign_slot(stack_at(frame.frame_pointer + op.index));
}

[[clang::noinline]]
void BytecodeVM::execute_op_for_loop(
    const bytecode::OpForLoop &op, CallFrame &frame
) {
  const auto control_slot = frame.frame_pointer + op.control_index;
  const auto limit_slot = frame.frame_pointer + op.limit_index;

  if (!stack_at(control_slot)->is_primitive() ||
      !stack_at(control_slot)->as_primitive()->get().is_integer()) {
    throw runtime::RuntimeError("for-loop control variable must be an integer");
  }
  if (!stack_at(limit_slot)->is_primitive() ||
      !stack_at(limit_slot)->as_primitive()->get().is_integer()) {
    throw runtime::RuntimeError("for-loop limit value must be an integer");
  }

  std::int64_t step = 1;
  if (op.step_index) {
    const auto step_slot = frame.frame_pointer + *op.step_index;
    if (!stack_at(step_slot)->is_primitive() ||
        !stack_at(step_slot)->as_primitive()->get().is_integer()) {
      throw runtime::RuntimeError("for-loop step value must be an integer");
    }
    step = stack_at(step_slot)->as_primitive()->get().as_integer().value();
  }

  const auto current =
      stack_at(control_slot)->as_primitive()->get().as_integer().value();
  const auto next = current + step;
  stack_at(control_slot) =
      store_value(runtime::Value{runtime::Primitive{static_cast<long>(next)}});

  const auto limit =
      stack_at(limit_slot)->as_primitive()->get().as_integer().value();

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
void BytecodeVM::execute_op_make_array(const bytecode::OpMakeArray &op) {
  std::vector<runtime::Ref> elements;
  elements.reserve(op.count);
  for (std::size_t i = 0; i < op.count; ++i) {
    elements.push_back(stack_pop());
  }
  std::ranges::reverse(elements);
  stack_push(std::move(elements));
  debug_print("MAKE_ARRAY count={} result={}", op.count, stack_top());
}

[[clang::noinline]]
void BytecodeVM::execute_op_get_index(const bytecode::OpGetIndex & /*op*/) {
  debug_print("GET_INDEX array={} index={}", stack_top(1), stack_top());
  auto index = stack_pop();
  auto array = stack_pop();

  stack_push_new(array->index(*index));
}

[[clang::noinline]]
void BytecodeVM::execute_op_set_index(const bytecode::OpSetIndex & /*op*/) {
  auto value = stack_pop();
  auto index = stack_pop();
  auto array = stack_pop();
  debug_print("SET_INDEX array={} index={} value={}", array, index, value);

  array->index_mut(*index) = value;
}

[[clang::noinline]]
void BytecodeVM::execute_op_call(const bytecode::OpCall &op) {
  auto args_base_ptr = stack.end() - static_cast<std::ptrdiff_t>(op.arg_count);
  std::vector<runtime::Ref> args(args_base_ptr, stack.end());
  stack.erase(args_base_ptr, stack.end());

  auto func_ref = stack_pop();
  debug_print("CALL func={} args={}", func_ref, args);

  if (!func_ref->is_function()) {
    throw runtime::RuntimeError("Attempted to call a non-function value");
  }

  const auto result =
      evaluate(func_ref, std::move(args), current_instruction_location());
  if (op.keep_return_value) {
    stack.push_back(result);
  }
}

namespace {
runtime::BytecodeFunctionId &get_mut_bytecode_function(runtime::Value &value) {
  return value.as_mut_function()->get()->as_mut_bytecode_function()->get();
}

const runtime::BytecodeFunctionId &
get_bytecode_function(const runtime::Value &value) {
  return value.as_function()->get()->as_bytecode_function()->get();
}
} // namespace

[[clang::noinline]]
void BytecodeVM::execute_op_closure(
    const bytecode::OpClosure &op, CallFrame &frame
) {
  auto function = get_mut_bytecode_function(
      current_program->constants[op.function_index].get_value_mut()
  );

  auto &upvalues = function.upvalues;

  for (const auto &[local, index] : op.upvalues) {
    if (local) {
      upvalues.push_back(frame.frame_pointer + index);
    } else {
      const auto &closure = get_bytecode_function(**frame.closure);
      upvalues.push_back(closure.upvalues[index]);
    }
  }
  stack_push(runtime::Function{std::move(function)});
  debug_print("CLOSURE function={}", stack.back());
}

[[clang::noinline]]
void BytecodeVM::execute_op_get_upvalue(
    const bytecode::OpGetUpvalue &op, CallFrame &frame
) {
  const auto &upvalues = get_bytecode_function(**frame.closure).upvalues;
  debug_print("upvalues: {}", upvalues);
  const auto ptr = upvalues[op.index];
  const auto val = stack[ptr];
  debug_print("GET_UPVALUE index={} ptr={} value={}", op.index, ptr, val);
  stack.push_back(val);
}

[[clang::noinline]]
void BytecodeVM::execute_op_set_upvalue(
    const bytecode::OpSetUpvalue &op, CallFrame &frame
) {
  debug_print("SET_UPVALUE index={}", op.index, stack_top());
  get_mut_bytecode_function(**frame.closure).upvalues[op.index] =
      stack.size() - 1;
}

} // namespace l3::vm
