module l3.vm;

import std;
import l3.ast;
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

template <typename Op>
void binary_op(
    BytecodeVM &vm, std::vector<runtime::StackValue> &stack, const Op &op
) {
  auto &a = stack[stack.size() - 2];
  auto &b = stack.back();
  a = vm.heap_store(op(a, b));
  stack.pop_back();
}

template <typename Op>
void unary_op(
    BytecodeVM &vm, std::vector<runtime::StackValue> &stack, const Op &op
) {
  auto &a = stack.back();
  a = vm.heap_store(op(a));
}

template <typename Pred>
void compare_op(std::vector<runtime::StackValue> &stack, const Pred &pred) {
  auto &a = stack[stack.size() - 2];
  auto &b = stack.back();
  a = {runtime::Primitive{pred(runtime::compare(a, b))}};
  stack.pop_back();
}

} // namespace

BytecodeVM::BytecodeVM(bool debug_) : debug(debug_) {
  for (const auto &[name, body] : l3::builtins::BUILTINS) {
    auto func = heap_store(
        runtime::Function{runtime::BuiltinFunction{
            ast::Identifier{std::string(name)},
            [this, body](runtime::L3Args args) -> runtime::Value {
              return body(*this, args);
            }
        }}
    );
    define_global(name, func);
  }
}

runtime::StackValue BytecodeVM::heap_store(runtime::Value &&value) {
  return std::move(value).visit(
      [](runtime::Nil) -> runtime::StackValue { return {}; },
      [](runtime::Primitive p) -> runtime::StackValue { return {p}; },
      [this](auto &f) -> runtime::StackValue {
        return {&heap.emplace(runtime::Value{std::move(f)})};
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

runtime::StackValue BytecodeVM::stack_pop() {
  auto val = stack.back();
  stack.pop_back();
  return val;
}

void BytecodeVM::stack_push(runtime::StackValue value) {
  stack.emplace_back(value);
}

std::optional<runtime::StackValue>
BytecodeVM::resolve_global(std::string_view name) const {
  if (auto it = global_symbols.find(name); it != global_symbols.end()) {
    return it->second;
  }
  return std::nullopt;
}

void BytecodeVM::define_global(
    std::string_view name, runtime::StackValue value
) {
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

runtime::StackValue BytecodeVM::call_function(
    const runtime::StackValue &function,
    runtime::L3Args arguments,
    const location::Location &call_location
) {
  if (!function.holds_heap_cell()) {
    throw runtime::RuntimeError("call_function: not a function");
  }
  auto *gcv = function.get_heap_ptr();
  return gcv->get_value_mut().visit(
      [&](runtime::Value::function_type &f) {
        return f->visit(
            [&](const runtime::BuiltinFunction &func) {
              return heap_store(func.invoke(arguments));
            },
            [&](runtime::BytecodeFunction &bc_func) {
              auto total_args = bc_func.curried_args.size() + arguments.size();

              if (total_args < bc_func.arity) {
                auto new_func = bc_func;
                new_func.curried_args.append_range(arguments);
                return heap_store(runtime::Function{std::move(new_func)});
              }
              if (total_args == bc_func.arity) {
                auto previous_frames = frames.size();
                auto fp = stack.size();
                frames.emplace_back(
                    bc_func.id,
                    0,
                    fp,
                    call_location,
                    std::pair{bc_func, function}
                );
                auto &new_frame = frames.back();
                new_frame.upvalues = bc_func.captured_upvalue_refs;
                stack.reserve(stack.size() + total_args);
                stack.append_range(bc_func.curried_args);
                stack.append_range(arguments);
                execute_loop(previous_frames);
                return stack_pop();
              }

              throw runtime::RuntimeError("call_function arity mismatch");
            }
        );
      },
      [&](auto &) -> runtime::StackValue {
        throw runtime::RuntimeError("call_function: not a function");
      }
  );
}

std::size_t BytecodeVM::run_gc() {
  static const auto mark_stack_value = [](runtime::StackValue &sv) {
    if (auto *gcv = sv.get_heap_ptr()) {
      gcv->mark();
    }
  };

  for (auto &sv : stack) {
    mark_stack_value(sv);
  }
  for (auto &[_, sv] : global_symbols) {
    if (auto *gcv = sv.get_heap_ptr()) {
      gcv->mark();
    }
  }
  for (auto &frame : frames) {
    if (frame.closure && frame.closure->second.holds_heap_cell()) {
      frame.closure->second.get_heap_ptr_mut()->mark();
    }
    for (auto *uv : frame.upvalues) {
      uv->mark();
    }
    for (auto &[_, uv] : frame.captured_locals) {
      uv->mark();
    }
  }
  if (current_program != nullptr) {
    for (auto &gc_val : current_program->constants) {
      gc_val.mark();
    }
  }
  heap.sweep();
  return upvalues.sweep();
}

void BytecodeVM::maybe_gc() {
  if (heap.get_added_since_last_sweep() >= heap.get_next_gc_threshold()) {
    run_gc();
  }
}

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
  stack.clear();
}

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

void BytecodeVM::
    execute_op(const bytecode::OpReturn & /*op*/, CallFrame & /*frame*/) {
  debug_print("RETURN value={}", stack_top());
  frames.pop_back();
}

void BytecodeVM::
    execute_op(const bytecode::OpConstant &op, CallFrame & /*frame*/) {
  runtime::HeapCell &chunk_val = constant_at(op.index);
  chunk_val.get_value_mut().visit(
      [&](runtime::Nil) { stack.emplace_back(); },
      [&](runtime::Primitive p) { stack.emplace_back(p); },
      [&](auto &) { stack.emplace_back(&chunk_val); }
  );
  debug_print("CONSTANT index={} value={}", op.index, stack_top());
}

void BytecodeVM::execute_op(const bytecode::OpPop &op, CallFrame & /*frame*/) {
  auto available = stack.size() - current_frame_pointer();
  if (available < op.count) {
    throw runtime::RuntimeError("stack underflow");
  }

  if (op.count == 1) {
    debug_print("POP value={}", stack_pop());
  } else {
    auto base = stack.end() - static_cast<std::ptrdiff_t>(op.count);
    stack.erase(base, stack.end());
  }
}

void BytecodeVM::
    execute_op(const bytecode::OpDuplicate &op, CallFrame & /*frame*/) {
  debug_print("DUPLICATE value={}", stack_top(op.index));
  stack.push_back(stack_top(op.index));
}

void BytecodeVM::
    execute_op(const bytecode::OpAdd & /*op*/, CallFrame & /*frame*/) {
  debug_print("ADD a={} b={}", stack_top(1), stack_top());
  binary_op(*this, stack, runtime::add);
}

void BytecodeVM::
    execute_op(const bytecode::OpSubtract & /*op*/, CallFrame & /*frame*/) {
  debug_print("SUBTRACT a={} b={}", stack_top(1), stack_top());
  binary_op(*this, stack, runtime::sub);
}

void BytecodeVM::
    execute_op(const bytecode::OpMultiply & /*op*/, CallFrame & /*frame*/) {
  debug_print("MULTIPLY a={} b={}", stack_top(1), stack_top());
  binary_op(*this, stack, runtime::mul);
}

void BytecodeVM::
    execute_op(const bytecode::OpDivide & /*op*/, CallFrame & /*frame*/) {
  debug_print("DIVIDE a={} b={}", stack_top(1), stack_top());
  binary_op(*this, stack, runtime::div);
}

void BytecodeVM::
    execute_op(const bytecode::OpModulo & /*op*/, CallFrame & /*frame*/) {
  debug_print("MODULO a={} b={}", stack_top(1), stack_top());
  binary_op(*this, stack, runtime::mod);
}

void BytecodeVM::
    execute_op(const bytecode::OpPower & /*op*/, CallFrame & /*frame*/) {
  debug_print("POWER a={} b={}", stack_top(1), stack_top());
  binary_op(*this, stack, runtime::pow);
}

void BytecodeVM::
    execute_op(const bytecode::OpNegate & /*op*/, CallFrame & /*frame*/) {
  debug_print("NEGATE a={}", stack_top());
  unary_op(*this, stack, runtime::negative);
}

void BytecodeVM::
    execute_op(const bytecode::OpNot & /*op*/, CallFrame & /*frame*/) {
  debug_print("NOT a={}", stack_top());
  unary_op(*this, stack, runtime::not_op);
}

void BytecodeVM::
    execute_op(const bytecode::OpEqual & /*op*/, CallFrame & /*frame*/) {
  debug_print("EQUAL a={} b={}", stack_top(1), stack_top());
  compare_op(stack, [](auto cmp) {
    return cmp == std::partial_ordering::equivalent;
  });
}

void BytecodeVM::
    execute_op(const bytecode::OpNotEqual & /*op*/, CallFrame & /*frame*/) {
  debug_print("NOT_EQUAL a={} b={}", stack_top(1), stack_top());
  compare_op(stack, [](auto cmp) {
    return cmp != std::partial_ordering::equivalent;
  });
}

void BytecodeVM::
    execute_op(const bytecode::OpGreater & /*op*/, CallFrame & /*frame*/) {
  debug_print("GREATER a={} b={}", stack_top(1), stack_top());
  compare_op(stack, [](auto cmp) {
    return cmp == std::partial_ordering::greater;
  });
}

void BytecodeVM::
    execute_op(const bytecode::OpGreaterEqual & /*op*/, CallFrame & /*frame*/) {
  debug_print("GREATER_EQUAL a={} b={}", stack_top(1), stack_top());
  compare_op(stack, [](auto cmp) {
    return cmp == std::partial_ordering::greater ||
           cmp == std::partial_ordering::equivalent;
  });
}

void BytecodeVM::
    execute_op(const bytecode::OpLess & /*op*/, CallFrame & /*frame*/) {
  debug_print("LESS a={} b={}", stack_top(1), stack_top());
  compare_op(stack, [](auto cmp) {
    return cmp == std::partial_ordering::less;
  });
}

void BytecodeVM::
    execute_op(const bytecode::OpLessEqual & /*op*/, CallFrame & /*frame*/) {
  debug_print("LESS_EQUAL a={} b={}", stack_top(1), stack_top());
  compare_op(stack, [](auto cmp) {
    return cmp == std::partial_ordering::less ||
           cmp == std::partial_ordering::equivalent;
  });
}

void BytecodeVM::execute_op(const bytecode::OpJump &op, CallFrame & /*frame*/) {
  debug_print("JUMP target={}", op.offset);
  current_frame().ip = op.offset;
}

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

void BytecodeVM::
    execute_op(const bytecode::OpGetGlobal &op, CallFrame & /*frame*/) {
  auto &name_gcv = constant_at(op.name_index);
  name_gcv.get_value().visit(
      [&](const std::string &name) {
        const auto slot = resolve_global(name);
        if (!slot) {
          throw runtime::UndefinedVariableError("{}", name);
        }
        const auto &sv = *slot;
        debug_print("GET_GLOBAL name={} value={}", name, sv);
        stack.push_back(sv);
      },
      [](const auto &) {
        throw runtime::RuntimeError("OpGetGlobal: constant is not a string");
      }
  );
}

void BytecodeVM::
    execute_op(const bytecode::OpSetGlobal &op, CallFrame & /*frame*/) {
  auto &name_gcv = constant_at(op.name_index);
  name_gcv.get_value().visit(
      [&](const std::string &name) {
        auto slot = resolve_global(name);
        if (!slot) {
          throw runtime::RuntimeError("Undefined variable: {}", name);
        }
        debug_print("SET_GLOBAL name={} value={}", name, stack_top());
        *slot = stack_pop();
      },
      [](const auto &) {
        throw runtime::RuntimeError("OpSetGlobal: constant is not a string");
      }
  );
}

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

void BytecodeVM::execute_op(const bytecode::OpSetLocal &op, CallFrame &frame) {
  debug_print("SET_LOCAL index={} value={}", op.index, stack_top());
  auto val = stack_pop();
  stack_at(frame.frame_pointer + op.index) = val;
  auto it = frame.captured_locals.find(op.index);
  if (it != frame.captured_locals.end()) {
    it->second->get() = val;
  }
}

void BytecodeVM::execute_op(const bytecode::OpForLoop &op, CallFrame &frame) {
  const auto control_slot = frame.frame_pointer + op.control_index;
  const auto limit_slot = frame.frame_pointer + op.limit_index;

  auto get_primitive = [&](std::size_t slot) -> std::int64_t {
    auto &sv = stack_at(slot);
    if (!sv.is_primitive()) {
      throw runtime::RuntimeError("for-loop requires integer values");
    }
    return sv.as_primitive()->get().visit([](const auto &v) {
      return static_cast<std::int64_t>(v);
    });
  };

  auto current = get_primitive(control_slot);
  auto next = current + 1;

  if (op.step_index) {
    auto step = get_primitive(frame.frame_pointer + *op.step_index);
    next = current + step;
  }

  stack_at(control_slot) = runtime::StackValue{runtime::Primitive{next}};

  auto limit = get_primitive(limit_slot);

  const bool keep_running = op.inclusive ? (next <= limit) : (next < limit);

  if (keep_running) {
    current_frame().ip = op.body_offset;
  }

  debug_print(
      "FOR_LOOP ctrl={} lim={} step={} next={} body={} take={}",
      op.control_index,
      op.limit_index,
      1,
      next,
      op.body_offset,
      keep_running
  );
}

void BytecodeVM::
    execute_op(const bytecode::OpMakeArray &op, CallFrame & /*frame*/) {
  const auto start = stack.size() - op.count;
  std::vector<runtime::StackValue> elements{
      stack.begin() + static_cast<std::ptrdiff_t>(start), stack.end()
  };
  stack.resize(start);
  debug_print("MAKE_ARRAY count={}", op.count);
  stack_push(heap_store(std::move(elements)));
}

void BytecodeVM::
    execute_op(const bytecode::OpGetIndex & /*op*/, CallFrame & /*frame*/) {
  auto &index_sv = stack.back();
  auto &array_sv = stack[stack.size() - 2];

  debug_print("GET_INDEX array={} index={}", array_sv, index_sv);

  auto result = runtime::index(array_sv, index_sv);
  stack.pop_back();
  match::match(
      std::move(result),
      [this](runtime::StackValue sv) { stack.back() = sv; },
      [this](runtime::Value &&value) {
        stack.back() = heap_store(std::move(value));
      }
  );
}

void BytecodeVM::
    execute_op(const bytecode::OpSetIndex & /*op*/, CallFrame & /*frame*/) {
  auto value_sv = stack_pop();
  auto index_sv = stack_pop();

  auto &array_sv = stack.back();
  debug_print(
      "SET_INDEX array={} index={} value={}", array_sv, index_sv, value_sv
  );

  runtime::index_mut(array_sv, index_sv) = value_sv;
  stack.pop_back();
}

void BytecodeVM::execute_op(const bytecode::OpCall &op, CallFrame & /*frame*/) {
  const auto func_pos = stack.size() - op.arg_count - 1;
  auto func_sv = stack[func_pos];
  stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(func_pos));

  debug_print("CALL func={} argc={}", func_sv, op.arg_count);

  const auto base = stack.size() - op.arg_count;
  const auto args_span = std::span(
      stack.end() - static_cast<std::ptrdiff_t>(op.arg_count), op.arg_count
  );

  const auto clean_args = [&]() {
    stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(base), stack.end());
  };

  if (!func_sv.holds_heap_cell()) {
    clean_args();
    throw runtime::RuntimeError("Attempted to call a non-function value");
  }

  auto *gcv = func_sv.get_heap_ptr();
  auto result = gcv->get_value_mut().visit(
      [&](runtime::Value::function_type &f) {
        return f->visit(
            [&](const runtime::BuiltinFunction &bf) {
              return heap_store(bf.invoke(args_span));
            },
            [&](runtime::BytecodeFunction &bc_func) {
              auto total_args = bc_func.curried_args.size() + op.arg_count;

              if (total_args > bc_func.arity) {
                throw runtime::RuntimeError("call_function arity mismatch");
              }

              if (total_args < bc_func.arity) {
                auto new_func = bc_func;
                new_func.curried_args.append_range(args_span);
                return heap_store(runtime::Function{std::move(new_func)});
              }

              auto previous_frames = frames.size();
              auto &new_frame = frames.emplace_back(
                  bc_func.id,
                  0,
                  base,
                  current_instruction_location(),
                  std::pair{bc_func, func_sv}
              );
              new_frame.upvalues = bc_func.captured_upvalue_refs;

              if (!bc_func.curried_args.empty()) {
                const auto curried_count = bc_func.curried_args.size();
                stack.resize(stack.size() + curried_count);

                for (std::size_t i = op.arg_count; i > 0; i--) {
                  stack[base + i - 1 + curried_count] = stack[base + i - 1];
                }

                for (const auto &[i, arg] :
                     utils::ranges::enumerate(bc_func.curried_args)) {
                  stack[base + i] = arg;
                }
              }

              execute_loop(previous_frames);
              return stack_pop();
            }
        );
      },
      [&](auto &) -> runtime::StackValue {
        clean_args();
        throw runtime::RuntimeError(
            "Attempted to call a non-function value: {}",
            gcv->get_value().type_name()
        );
      }
  );

  clean_args();

  if (op.keep_return_value) {
    stack_push(result);
  }
}

void BytecodeVM::execute_op(const bytecode::OpClosure &op, CallFrame &frame) {
  auto &constant = current_program->constants[op.function_index];
  auto *func_ptr = constant.get_value_mut().visit(
      [](runtime::Value::function_type &f) -> runtime::BytecodeFunction * {
        if (auto bc_opt = f->as_mut_bytecode_function()) {
          return &bc_opt->get();
        }
        return nullptr;
      },
      [](auto &) -> runtime::BytecodeFunction * { return nullptr; }
  );

  if (func_ptr == nullptr) {
    throw runtime::RuntimeError("OpClosure: constant is not a function");
  }

  auto &function = *func_ptr;

  for (const auto &[local, index] : op.upvalues) {
    if (local) {
      auto it = frame.captured_locals.find(index);
      if (it == frame.captured_locals.end()) {
        it = frame.captured_locals
                 .emplace(index, &upvalues.emplace(stack_local(index)))
                 .first;
      }
      function.captured_upvalue_refs.push_back(it->second);
    } else {
      function.captured_upvalue_refs.push_back(
          frame.closure->first.captured_upvalue_refs[index]
      );
    }
  }
  stack_push(heap_store(std::move(function)));
  debug_print("CLOSURE function={}", stack.back());
}

void BytecodeVM::execute_op(
    const bytecode::OpGetUpvalue &op, CallFrame &frame
) {
  const auto &val = frame.upvalues[op.index]->get();
  debug_print("GET_UPVALUE index={} value={}", op.index, val);
  stack.push_back(val);
}

void BytecodeVM::execute_op(
    const bytecode::OpSetUpvalue &op, CallFrame &frame
) {
  debug_print("SET_UPVALUE index={} value={}", op.index, stack_top());
  frame.upvalues[op.index]->get() = stack_pop();
}

} // namespace l3::vm
