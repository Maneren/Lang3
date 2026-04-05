module l3.vm;

import std;
import l3.bytecode;
import l3.runtime;
import utils;

namespace l3::vm {

BytecodeVM::BytecodeVM(bool debug_) : debug(debug_) {
  for (const auto &[name, body] : l3::builtins::BUILTINS) {
    auto func = store_value(
        runtime::Function{runtime::BuiltinFunction{
            runtime::Identifier{std::string(name)},
            [this, &body](runtime::L3Args args) { return body(*this, args); }
        }}
    );
    define_file_scope_slot(name, func);
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

runtime::Ref BytecodeVM::stack_pop() {
  auto val = stack.back();
  stack.pop_back();
  return val;
}

void BytecodeVM::stack_push(runtime::Value &&value) {
  stack.emplace_back(gc_storage.emplace(std::move(value)));
}

std::optional<std::size_t>
BytecodeVM::resolve_file_scope_slot(std::string_view name) const {
  if (auto it = file_scope_indices.find(std::string{name});
      it != file_scope_indices.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::size_t
BytecodeVM::define_file_scope_slot(std::string_view name, runtime::Ref value) {
  auto key = std::string{name};
  if (auto it = file_scope_indices.find(key); it != file_scope_indices.end()) {
    file_scope_slots[it->second] = value;
    return it->second;
  }

  const auto index = file_scope_slots.size();
  file_scope_slots.push_back(value);
  file_scope_indices.emplace(std::move(key), index);
  return index;
}

runtime::Ref &BytecodeVM::file_scope_slot(std::size_t index) {
  return file_scope_slots[index];
}

const runtime::Ref &BytecodeVM::file_scope_slot(std::size_t index) const {
  return file_scope_slots[index];
}

void BytecodeVM::assign_slot(runtime::Ref &slot) {
  slot = stack.back();
  stack.pop_back();
}

void BytecodeVM::mutate_slot(runtime::Ref &slot) {
  slot.get() = std::move(stack.back().get());
  stack.pop_back();
}

template <typename... Args>
void BytecodeVM::debug_print(std::format_string<Args...> fmt, Args &&...args) {
  if (debug) {
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

runtime::Ref
BytecodeVM::evaluate(runtime::Ref function, runtime::L3Args arguments) {
  if (auto func_opt = function->as_function()) {
    if (auto bc_opt = func_opt->get()->as_bytecode_function()) {
      auto bc_func = bc_opt->get();
      auto total_args = bc_func.curried_args.size() + arguments.size();

      if (total_args < bc_func.arity) {
        auto new_func = bc_func;
        new_func.curried_args.insert(
            new_func.curried_args.end(), arguments.begin(), arguments.end()
        );
        return store_value(
            runtime::Value{runtime::Function{std::move(new_func)}}
        );
      }
      if (total_args == bc_func.arity) {
        auto previous_frames = frames.size();
        auto fp = stack.size();
        frames.emplace_back(bc_func.id, 0, fp, std::make_optional(function));
        for (const auto &arg : bc_func.curried_args) {
          stack.push_back(arg);
        }
        for (const auto &arg : arguments) {
          stack.push_back(arg);
        }
        execute_loop(previous_frames);
        return stack_pop();
      }

      throw std::runtime_error("evaluate arity mismatch");
    }

    if (auto builtin_opt = func_opt->get()->as_builtin_function()) {
      return builtin_opt->get().invoke(arguments);
    }
  }
  throw std::runtime_error("evaluate: not a function");
}

std::size_t BytecodeVM::run_gc() {
  for (auto &ref : stack) {
    ref.get_gc_mut().mark();
  }
  for (auto &ref : file_scope_slots) {
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
  constexpr std::size_t GC_INTERVAL = 10000;
  if (gc_storage.get_added_since_last_sweep() > GC_INTERVAL) {
    run_gc();
  }
}

void BytecodeVM::execute(const bytecode::ProgramBytecode &program) {
  current_program = &program;
  frames.emplace_back();
  execute_loop(0);
  current_program = nullptr;
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

    match::match(
        instruction,
        [&](const bytecode::OpReturn &op) { execute_op_return(op); },
        [&](const bytecode::OpConstant &op) { execute_op_constant(op); },
        [&](const bytecode::OpPop &op) { execute_op_pop(op); },
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
        [&](const bytecode::OpTest &op) { execute_op_jump_if_false(op); },
        [&](const bytecode::OpGetGlobal &op) { execute_op_get_global(op); },
        [&](const bytecode::OpSetGlobal &op) { execute_op_set_global(op); },
        [&](const bytecode::OpGetLocal &op) {
          execute_op_get_local(op, frame);
        },
        [&](const bytecode::OpSetLocal &op) {
          execute_op_set_local(op, frame);
        },
        [&](const bytecode::OpMutateLocal &op) {
          execute_op_mutate_local(op, frame);
        },
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

void BytecodeVM::execute_op_return(const bytecode::OpReturn & /*op*/) {
  auto result = stack_pop();
  debug_print("RETURN value={}", result);
  auto fp = frames.back().frame_pointer;
  frames.pop_back();
  stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(fp), stack.end());
  stack.push_back(result);
}

void BytecodeVM::execute_op_constant(const bytecode::OpConstant &op) {
  const auto &chunk_val = current_program->constants[op.index];
  if (chunk_val.is_primitive()) {
    stack_push(chunk_val.as_primitive()->get());
  } else if (chunk_val.is_string()) {
    stack_push(std::string(chunk_val.as_string()->get()));
  } else if (auto func_opt = chunk_val.as_function()) {
    if (auto bc_opt = func_opt->get()->as_bytecode_function()) {
      stack_push(runtime::Function{runtime::BytecodeFunctionId{bc_opt->get()}});
    } else if (auto builtin_opt = func_opt->get()->as_builtin_function()) {
      stack_push(
          runtime::Function{runtime::BuiltinFunction{builtin_opt->get()}}
      );
    }
  } else {
    stack_push(runtime::Nil{});
  }
  debug_print("CONSTANT index={} value={}", op.index, stack.back());
}

void BytecodeVM::execute_op_pop(const bytecode::OpPop & /*op*/) {
  if (!stack.empty()) {
    debug_print("POP value={}", stack_pop());
  }
}

void BytecodeVM::execute_op_add(const bytecode::OpAdd & /*op*/) {
  debug_print("ADD a={} b={}", stack[stack.size() - 2], stack.back());
  binary_arithmetic([](auto &a, auto &b) { return a.add(b); });
}

void BytecodeVM::execute_op_subtract(const bytecode::OpSubtract & /*op*/) {
  debug_print("SUBTRACT a={} b={}", stack[stack.size() - 2], stack.back());
  binary_arithmetic([](auto &a, auto &b) { return a.sub(b); });
}

void BytecodeVM::execute_op_multiply(const bytecode::OpMultiply & /*op*/) {
  debug_print("MULTIPLY a={} b={}", stack[stack.size() - 2], stack.back());
  binary_arithmetic([](auto &a, auto &b) { return a.mul(b); });
}

void BytecodeVM::execute_op_divide(const bytecode::OpDivide & /*op*/) {
  debug_print("DIVIDE a={} b={}", stack[stack.size() - 2], stack.back());
  binary_arithmetic([](auto &a, auto &b) { return a.div(b); });
}

void BytecodeVM::execute_op_modulo(const bytecode::OpModulo & /*op*/) {
  debug_print("MODULO a={} b={}", stack[stack.size() - 2], stack.back());
  binary_arithmetic([](auto &a, auto &b) { return a.mod(b); });
}

void BytecodeVM::execute_op_negate(const bytecode::OpNegate & /*op*/) {
  debug_print("NEGATE a={}", stack.back());
  auto a = stack_pop();
  stack_push(a->negative());
}

void BytecodeVM::execute_op_not(const bytecode::OpNot & /*op*/) {
  debug_print("NOT a={}", stack.back());
  auto a = stack_pop();
  stack_push(a->not_op());
}

void BytecodeVM::execute_op_equal(const bytecode::OpEqual & /*op*/) {
  debug_print("EQUAL a={} b={}", stack[stack.size() - 2], stack.back());
  comparison_op([](auto c) { return c == std::partial_ordering::equivalent; });
}

void BytecodeVM::execute_op_not_equal(const bytecode::OpNotEqual & /*op*/) {
  debug_print("NOT_EQUAL a={} b={}", stack[stack.size() - 2], stack.back());
  comparison_op([](auto c) { return c != std::partial_ordering::equivalent; });
}

void BytecodeVM::execute_op_greater(const bytecode::OpGreater & /*op*/) {
  debug_print("GREATER a={} b={}", stack[stack.size() - 2], stack.back());
  comparison_op([](auto c) { return c == std::partial_ordering::greater; });
}

void BytecodeVM::
    execute_op_greater_equal(const bytecode::OpGreaterEqual & /*op*/) {
  debug_print("GREATER_EQUAL a={} b={}", stack[stack.size() - 2], stack.back());
  comparison_op([](auto c) {
    return c == std::partial_ordering::greater ||
           c == std::partial_ordering::equivalent;
  });
}

void BytecodeVM::execute_op_less(const bytecode::OpLess & /*op*/) {
  debug_print("LESS a={} b={}", stack[stack.size() - 2], stack.back());
  comparison_op([](auto c) { return c == std::partial_ordering::less; });
}

void BytecodeVM::execute_op_less_equal(const bytecode::OpLessEqual & /*op*/) {
  debug_print("LESS_EQUAL a={} b={}", stack[stack.size() - 2], stack.back());
  comparison_op([](auto c) {
    return c == std::partial_ordering::less ||
           c == std::partial_ordering::equivalent;
  });
}

void BytecodeVM::execute_op_jump(const bytecode::OpJump &op) {
  debug_print("JUMP target={}", op.offset);
  frames.back().ip = op.offset;
}

void BytecodeVM::execute_op_jump_if_false(const bytecode::OpTest &op) {
  const bool take = stack.back()->is_falsy();
  debug_print(
      "TEST condition={} take={} target={}", stack.back(), take, op.offset
  );
  if (take) {
    frames.back().ip = op.offset;
  }
}

void BytecodeVM::execute_op_get_global(const bytecode::OpGetGlobal &op) {
  const auto &name_val = current_program->constants[op.name_index];
  if (auto str_opt = name_val.as_string()) {
    auto slot_index = resolve_file_scope_slot(str_opt->get());
    if (!slot_index) {
      throw std::runtime_error("Undefined variable: " + str_opt->get());
    }
    const auto &value = file_scope_slot(*slot_index);
    debug_print("GET_GLOBAL name={} value={}", str_opt->get(), value);
    stack.push_back(value);
  }
}

void BytecodeVM::execute_op_set_global(const bytecode::OpSetGlobal &op) {
  const auto &name_val = current_program->constants[op.name_index];
  if (auto str_opt = name_val.as_string()) {
    auto slot_index = resolve_file_scope_slot(str_opt->get());
    if (!slot_index) {
      throw std::runtime_error("Undefined variable: " + str_opt->get());
    }
    debug_print("SET_GLOBAL name={} value={}", str_opt->get(), stack.back());
    assign_slot(file_scope_slot(*slot_index));
  }
}

void BytecodeVM::execute_op_get_local(
    const bytecode::OpGetLocal &op, CallFrame &frame
) {
  stack.push_back(stack[frame.frame_pointer + op.index]);
  debug_print(
      "GET_LOCAL index={} fp={} stack size={} value={}",
      op.index,
      frame.frame_pointer,
      stack.size(),
      stack.back()
  );
}

void BytecodeVM::execute_op_set_local(
    const bytecode::OpSetLocal &op, CallFrame &frame
) {
  debug_print("SET_LOCAL index={} value={}", op.index, stack.back());
  assign_slot(stack[frame.frame_pointer + op.index]);
}

void BytecodeVM::execute_op_mutate_local(
    const bytecode::OpMutateLocal &op, CallFrame &frame
) {
  debug_print("MUTATE_LOCAL index={} value={}", op.index, stack.back());
  mutate_slot(stack[frame.frame_pointer + op.index]);
}

void BytecodeVM::execute_op_make_array(const bytecode::OpMakeArray &op) {
  std::vector<runtime::Ref> elements;
  elements.reserve(op.count);
  for (std::size_t i = 0; i < op.count; ++i) {
    elements.push_back(stack_pop());
  }
  std::ranges::reverse(elements);
  stack_push(std::move(elements));
  debug_print("MAKE_ARRAY count={} result={}", op.count, stack.back());
}

void BytecodeVM::execute_op_get_index(const bytecode::OpGetIndex & /*op*/) {
  debug_print(
      "GET_INDEX array={} index={}", stack[stack.size() - 2], stack.back()
  );
  auto index = stack_pop();
  auto array = stack_pop();
  if (auto vec_opt = array->as_vector()) {
    if (index->is_primitive() && index->as_primitive()->get().is_integer()) {
      auto idx = index->as_primitive()->get().as_integer().value();
      if (idx < 0 || idx >= static_cast<std::int64_t>(vec_opt->get().size())) {
        throw std::runtime_error("Array index out of bounds");
      }
      stack.push_back(vec_opt->get()[static_cast<std::size_t>(idx)]);
      return;
    }
  } else if (auto str_opt = array->as_string()) {
    if (index->is_primitive() && index->as_primitive()->get().is_integer()) {
      auto idx = index->as_primitive()->get().as_integer().value();
      if (idx < 0 || idx >= static_cast<std::int64_t>(str_opt->get().size())) {
        throw std::runtime_error("String index out of bounds");
      }
      std::string s(1, str_opt->get()[static_cast<std::size_t>(idx)]);
      stack_push(std::move(s));
      return;
    }
  }
  throw std::runtime_error("Invalid container or index for OpGetIndex");
}

void BytecodeVM::execute_op_set_index(const bytecode::OpSetIndex & /*op*/) {
  debug_print(
      "SET_INDEX array={} index={} value={}",
      stack[stack.size() - 3],
      stack[stack.size() - 2],
      stack.back()
  );
  auto value = stack_pop();
  auto index = stack_pop();
  auto array = stack_pop();
  if (auto vec_opt = array->as_mut_vector()) {
    if (index->is_primitive() && index->as_primitive()->get().is_integer()) {
      auto idx = index->as_primitive()->get().as_integer().value();
      if (idx < 0 || idx >= static_cast<std::int64_t>(vec_opt->get().size())) {
        throw std::runtime_error("Array index out of bounds");
      }
      vec_opt->get()[static_cast<std::size_t>(idx)] = value;
      stack.push_back(value);
      return;
    }
  }
  throw std::runtime_error("Invalid container or index for OpSetIndex");
}

void BytecodeVM::execute_op_call(const bytecode::OpCall &op) {
  std::vector<runtime::Ref> args;
  args.reserve(op.arg_count);
  for (std::size_t i = 0; i < op.arg_count; ++i) {
    args.push_back(stack_pop());
  }
  std::ranges::reverse(args);

  auto func_ref = stack_pop();
  debug_print("CALL func={} arg_count={}", func_ref, op.arg_count);

  if (auto func_opt = func_ref->as_function()) {
    const auto &func_ptr = func_opt->get();

    if (auto builtin_opt = func_ptr->as_builtin_function()) {
      auto result = builtin_opt->get().invoke(args);
      stack.push_back(result);
    } else if (auto bc_opt = func_ptr->as_bytecode_function()) {
      const auto &bc_func = bc_opt->get();
      auto total_args = bc_func.curried_args.size() + args.size();

      if (total_args < bc_func.arity) {
        debug_print("CALL curry: have={} need={}", total_args, bc_func.arity);
        auto new_func = bc_func;
        new_func.curried_args.insert(
            new_func.curried_args.end(), args.begin(), args.end()
        );
        stack_push(runtime::Function{std::move(new_func)});
      } else if (total_args == bc_func.arity) {
        std::size_t fp = stack.size();
        for (const auto &arg : bc_func.curried_args) {
          stack.push_back(arg);
        }
        for (const auto &arg : args) {
          stack.push_back(arg);
        }
        frames.push_back(
            {.chunk_id = bc_func.id,
             .ip = 0,
             .frame_pointer = fp,
             .closure = func_ref}
        );
      } else {
        throw std::runtime_error(
            "Arity mismatch: " + bc_func.name + " expected " +
            std::to_string(bc_func.arity) + " got " + std::to_string(total_args)
        );
      }
    } else {
      std::unreachable();
    }
  } else {
    throw std::runtime_error("Attempted to call a non-function value");
  }
}

void BytecodeVM::execute_op_closure(
    const bytecode::OpClosure &op, CallFrame &frame
) {
  const auto &chunk_val = current_program->constants[op.function_index];
  if (auto func_opt = chunk_val.as_function()) {
    if (auto bc_opt = func_opt->get()->as_bytecode_function()) {
      auto func_id = bc_opt->get();
      for (const auto &uv_info : op.upvalues) {
        if (uv_info.is_local) {
          func_id.upvalues.push_back(
              stack[frame.frame_pointer + uv_info.index]
          );
        } else {
          func_id.upvalues.push_back(frame.closure->get()
                                         .as_mut_function()
                                         ->get()
                                         ->as_mut_bytecode_function()
                                         ->get()
                                         .upvalues[uv_info.index]);
        }
      }
      stack_push(runtime::Function{std::move(func_id)});
      debug_print("CLOSURE function={}", stack.back());
    }
  }
}

void BytecodeVM::execute_op_get_upvalue(
    const bytecode::OpGetUpvalue &op, CallFrame &frame
) {
  auto val = frame.closure->get()
                 .as_mut_function()
                 ->get()
                 ->as_mut_bytecode_function()
                 ->get()
                 .upvalues[op.index];
  debug_print("GET_UPVALUE index={} value={}", op.index, val);
  stack.push_back(val);
}

void BytecodeVM::execute_op_set_upvalue(
    const bytecode::OpSetUpvalue &op, CallFrame &frame
) {
  auto val = stack.back();
  debug_print("SET_UPVALUE index={} value={}", op.index, val);
  frame.closure->get()
      .as_mut_function()
      ->get()
      ->as_mut_bytecode_function()
      ->get()
      .upvalues[op.index] = val;
}

} // namespace l3::vm
