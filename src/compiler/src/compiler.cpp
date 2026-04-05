module l3.compiler;

import std;
import l3.ast;
import l3.bytecode;
import l3.runtime;
import utils;

namespace l3::compiler {

using namespace l3::bytecode;

Compiler::Compiler(bytecode::ProgramBytecode &program) : program(program) {}

void Compiler::compile(const ast::Program &program) {
  this->program.chunks.emplace_back();
  current_chunk_id = this->program.chunks.size() - 1;
  for (const auto &stmt : program.get_statements()) {
    compile_statement(stmt);
  }
  if (auto last = program.get_last_statement(); last) {
    throw std::runtime_error("Unexpected last statement in top-level scope");
  }
  emit(OpConstant{make_constant({})});
  emit(OpReturn{});
  deduplicate_constants();
}

Chunk &Compiler::current_chunk() { return program.chunks[current_chunk_id]; }
std::size_t Compiler::last_instruction_offset() {
  return current_instruction_offset() - 1;
}
std::size_t Compiler::current_instruction_offset() {
  return current_chunk().code.size();
}

std::size_t Compiler::push_context() {
  contexts.emplace_back(
      current_chunk_id,
      std::move(locals),
      scope_depth,
      std::move(current_upvalues)
  );
  current_chunk_id = program.chunks.size();
  program.chunks.emplace_back();
  locals.clear();
  scope_depth = 0;
  current_upvalues.clear();
  return current_chunk_id;
}

void Compiler::pop_context() {
  auto ctx = std::move(contexts.back());
  contexts.pop_back();
  current_chunk_id = ctx.chunk_id;
  locals = std::move(ctx.locals);
  scope_depth = ctx.scope_depth;
  current_upvalues = std::move(ctx.upvalues);
}

void Compiler::begin_scope() { scope_depth++; }

void Compiler::end_scope() {
  scope_depth--;
  while (!locals.empty() && locals.back().depth > scope_depth) {
    emit(OpPop{});
    locals.pop_back();
  }
}

std::size_t Compiler::add_local(const ast::Identifier &name) {
  auto index = locals.size();
  locals.emplace_back(name, scope_depth);
  return index;
}

std::optional<std::size_t>
Compiler::resolve_local(const ast::Identifier &name) {
  for (const auto &[i, local] :
       std::views::reverse(utils::ranges::enumerate(locals))) {
    if (local.name == name) {
      return i;
    }
  }
  return std::nullopt;
}

std::optional<std::size_t> Compiler::resolve_local_in_context(
    const ast::Identifier &name, const Context &ctx
) {
  for (const auto &[i, local] :
       std::views::reverse(utils::ranges::enumerate(ctx.locals))) {
    if (local.name == name) {
      return i;
    }
  }
  return std::nullopt;
}

std::size_t Compiler::add_upvalue(
    std::vector<Upvalue> &upvalues, bool is_local, std::size_t index
) {
  for (const auto &[i, uv] : utils::ranges::enumerate(upvalues)) {
    if (uv.is_local == is_local && uv.index == index) {
      return i;
    }
  }
  upvalues.push_back({.is_local = is_local, .index = index});
  return upvalues.size() - 1;
}

std::optional<std::size_t> Compiler::resolve_upvalue(
    const ast::Identifier &name, std::size_t context_index
) {

  std::optional<std::size_t> local =
      resolve_local_in_context(name, contexts[context_index]);

  if (local) {
    std::size_t upvalue_idx =
        (context_index + 1 == contexts.size())
            ? add_upvalue(current_upvalues, true, *local)
            : add_upvalue(contexts[context_index + 1].upvalues, true, *local);

    for (std::size_t j = context_index + 2; j <= contexts.size(); j++) {
      if (j == contexts.size()) {
        upvalue_idx = add_upvalue(current_upvalues, false, upvalue_idx);
      } else {
        upvalue_idx = add_upvalue(contexts[j].upvalues, false, upvalue_idx);
      }
    }
    return upvalue_idx;
  }

  if (context_index == 0) {
    return std::nullopt;
  }

  return resolve_upvalue(name, context_index - 1);
}

Compiler::ResolvedVariable
Compiler::resolve_variable(const ast::Identifier &identifier) {
  if (auto local_arg = resolve_local(identifier)) {
    return {.type = Compiler::VariableType::Local, .index = *local_arg};
  }

  if (!contexts.empty()) {
    if (auto upvalue_arg = resolve_upvalue(identifier, contexts.size() - 1)) {
      return {.type = Compiler::VariableType::Upvalue, .index = *upvalue_arg};
    }
  }

  return {
      .type = Compiler::VariableType::Global,
      .index = make_constant(runtime::Value{std::string{identifier.get_name()}})
  };
}

Instruction Compiler::emit_get_variable(const ast::Identifier &name) {
  auto resolved = resolve_variable(name);
  switch (resolved.type) {
  case VariableType::Local:
    return OpGetLocal{resolved.index};
  case VariableType::Upvalue:
    return OpGetUpvalue{resolved.index};
  case VariableType::Global:
    return OpGetGlobal{resolved.index};
  }
}

Instruction Compiler::emit_set_variable(const ast::Identifier &name) {
  auto resolved = resolve_variable(name);
  switch (resolved.type) {
  case VariableType::Local:
    return OpSetLocal{resolved.index};
  case VariableType::Upvalue:
    return OpSetUpvalue{resolved.index};
  case VariableType::Global:
    return OpSetGlobal{resolved.index};
  }
}

void Compiler::emit(const Instruction &instruction, std::size_t line) {
  current_chunk().write(instruction, line);
}

std::size_t Compiler::make_constant(runtime::Value &&value) {
  auto index = program.constants.size();
  program.constants.emplace_back(std::move(value));
  return index;
}

void Compiler::deduplicate_constants() {
  std::vector<std::size_t> index_map(program.constants.size(), 0UZ);
  std::unordered_map<std::string, std::size_t> string_indices;
  std::vector<runtime::Value> deduped_constants;
  deduped_constants.reserve(program.constants.size());

  for (std::size_t old_index = 0; old_index < program.constants.size();
       ++old_index) {
    auto &constant = program.constants[old_index];
    if (const auto string_value = constant.as_string()) {
      const auto [it, inserted] =
          string_indices.emplace(string_value->get(), deduped_constants.size());
      if (inserted) {
        deduped_constants.emplace_back(runtime::Value{std::string{it->first}});
      }
      index_map[old_index] = it->second;
      continue;
    }

    index_map[old_index] = deduped_constants.size();
    deduped_constants.emplace_back(std::move(constant));
  }

  for (auto &chunk : program.chunks) {
    for (auto &instruction : chunk.code) {
      match::match(
          instruction,
          [&index_map](OpConstant &op) { op.index = index_map[op.index]; },
          [&index_map](OpGetGlobal &op) {
            op.name_index = index_map[op.name_index];
          },
          [&index_map](OpSetGlobal &op) {
            op.name_index = index_map[op.name_index];
          },
          [&index_map](OpClosure &op) {
            op.function_index = index_map[op.function_index];
          },
          [](auto &) {}
      );
    }
  }

  program.constants = std::move(deduped_constants);
}

void Compiler::emit_loop(std::size_t loop_start) { emit(OpJump{loop_start}); }

void Compiler::patch_jump(std::size_t jump_offset, std::size_t target) {
  match::match(
      current_chunk().code[jump_offset],
      [=](OpJump &jump) { jump.offset = target; },
      [=](OpTest &jump) { jump.offset = target; },
      [](auto &) {
        throw std::runtime_error("Attempting to patch a non-jump instruction");
      }
  );
}

void Compiler::patch_jump_here(std::size_t jump_offset) {
  patch_jump(jump_offset, last_instruction_offset() + 1);
}

std::size_t Compiler::emit_jump(const Instruction &instruction) {
  current_chunk().write(instruction, 0);
  return last_instruction_offset();
}

void Compiler::compile_block(const ast::Block &block) {
  begin_scope();
  for (const auto &stmt : block.get_statements()) {
    compile_statement(stmt);
  }
  if (auto last = block.get_last_statement(); last) {
    compile_last_statement(last->get());
  }
  end_scope();
}

void Compiler::compile_expression(const ast::Expression &expr) {
  expr.visit(
      [this](const ast::Literal &literal) { compile_literal(literal); },
      [this](const ast::BinaryExpression &binary) {
        compile_binary_expression(binary);
      },
      [this](const ast::UnaryExpression &unary) {
        compile_unary_expression(unary);
      },
      [this](const ast::LogicalExpression &logical) {
        compile_logical_expression(logical);
      },
      [this](const ast::Comparison &comparison) {
        compile_comparison(comparison);
      },
      [this](const ast::Variable &variable) { compile_variable(variable); },
      [this](const ast::AnonymousFunction &func) {
        compile_anonymous_function(func);
      },
      [this](const ast::FunctionCall &call) { compile_function_call(call); },
      [this](const ast::IfExpression &if_expr) {
        compile_if_expression(if_expr);
      }
  );
}

void Compiler::compile_binary_expression(const ast::BinaryExpression &binary) {
  compile_expression(binary.get_lhs());
  compile_expression(binary.get_rhs());

  switch (binary.get_op()) {
  case ast::BinaryOperator::Plus:
    emit(OpAdd{});
    break;
  case ast::BinaryOperator::Minus:
    emit(OpSubtract{});
    break;
  case ast::BinaryOperator::Multiply:
    emit(OpMultiply{});
    break;
  case ast::BinaryOperator::Divide:
    emit(OpDivide{});
    break;
  case ast::BinaryOperator::Modulo:
    emit(OpModulo{});
    break;
  case ast::BinaryOperator::Power: /* emit(OpPower{}); */
    break;
  }
}

void Compiler::compile_unary_expression(const ast::UnaryExpression &unary) {
  compile_expression(unary.get_expression());

  switch (unary.get_op()) {
  case ast::UnaryOperator::Minus:
    emit(OpNegate{});
    break;
  case ast::UnaryOperator::Not:
    emit(OpNot{});
    break;
  case ast::UnaryOperator::Plus: /* do nothing */
    break;
  }
}

void Compiler::compile_logical_expression(
    const ast::LogicalExpression &logical
) {
  compile_expression(logical.get_lhs());

  switch (logical.get_op()) {
  case ast::LogicalOperator::And: {
    std::size_t jump = emit_jump(OpTest{});
    emit(OpPop{});
    compile_expression(logical.get_rhs());
    patch_jump_here(jump);
  } break;
  case ast::LogicalOperator::Or: { // Logical OR
    std::size_t jump_if_false = emit_jump(OpTest{});
    std::size_t jump_to_end = emit_jump(OpJump{});

    patch_jump_here(jump_if_false);
    emit(OpPop{});
    compile_expression(logical.get_rhs());

    patch_jump_here(jump_to_end);
  } break;
  }
}

void Compiler::compile_comparison(const ast::Comparison &comparison) {
  const auto &comps = comparison.get_comparisons();
  if (comps.empty()) {
    compile_expression(comparison.get_start());
    return;
  }

  std::vector<std::size_t> jumps;

  for (std::size_t i = 0; i < comps.size(); ++i) {
    if (i == 0) {
      compile_expression(comparison.get_start());
    } else {
      compile_expression(comps[i - 1].second);
    }

    compile_expression(comps[i].second);

    switch (comps[i].first) {
    case ast::ComparisonOperator::Equal:
      emit(OpEqual{});
      break;
    case ast::ComparisonOperator::NotEqual:
      emit(OpNotEqual{});
      break;
    case ast::ComparisonOperator::Greater:
      emit(OpGreater{});
      break;
    case ast::ComparisonOperator::GreaterEqual:
      emit(OpGreaterEqual{});
      break;
    case ast::ComparisonOperator::Less:
      emit(OpLess{});
      break;
    case ast::ComparisonOperator::LessEqual:
      emit(OpLessEqual{});
      break;
    }

    if (i < comps.size() - 1) {
      jumps.push_back(emit_jump(OpTest{}));
      emit(OpPop{});
    }
  }

  for (auto jump : jumps) {
    patch_jump_here(jump);
  }
}

void Compiler::compile_anonymous_function(const ast::AnonymousFunction &func) {
  const auto new_chunk_id = push_context();

  const auto &args = func.get_body().get_parameters();
  for (const auto &arg : args) {
    add_local(arg);
  }

  compile_block(func.get_body().get_block());
  emit(OpConstant{make_constant({})});
  emit(OpReturn{});

  std::vector<UpvalueInfo> upvalue_infos;
  upvalue_infos.reserve(current_upvalues.size());
  for (const auto &uv : current_upvalues) {
    upvalue_infos.push_back({.is_local = uv.is_local, .index = uv.index});
  }

  pop_context();

  std::size_t id = make_constant(
      runtime::Function{runtime::BytecodeFunctionId{
          .id = new_chunk_id,
          .name = "<anonymous>",
          .arity = args.size(),
          .upvalues = {},
          .curried_args = {}
      }}
  );

  if (upvalue_infos.empty()) {
    emit(OpConstant{id});
  } else {
    emit(OpClosure{.function_index = id, .upvalues = std::move(upvalue_infos)});
  }
}

void Compiler::compile_function_call(const ast::FunctionCall &call) {
  compile_variable(ast::Variable{ast::Identifier{call.get_name().get_name()}});
  for (const auto &arg : call.get_arguments()) {
    compile_expression(arg);
  }
  emit(OpCall{call.get_arguments().size()});
}

void Compiler::compile_if_expression(const ast::IfExpression &if_expr) {
  std::vector<std::size_t> jump_to_ends;

  const auto &base_if = if_expr.get_base_if();

  compile_expression(base_if.get_condition());
  std::size_t then_jump = emit_jump(OpTest{});
  emit(OpPop{});

  compile_block(base_if.get_block());

  emit(OpConstant{make_constant({})});
  jump_to_ends.push_back(emit_jump(OpJump{}));

  patch_jump_here(then_jump);
  emit(OpPop{});

  for (const auto &elif : if_expr.get_elseif().get_elseifs()) {
    compile_expression(elif.get_condition());
    std::size_t elif_jump = emit_jump(OpTest{});

    emit(OpPop{});
    compile_block(elif.get_block());
    emit(OpConstant{make_constant({})});
    jump_to_ends.push_back(emit_jump(OpJump{}));

    patch_jump_here(elif_jump);
    emit(OpPop{});
  }

  compile_block(if_expr.get_else_block());
  emit(OpConstant{make_constant({})});

  for (auto jump : jump_to_ends) {
    patch_jump_here(jump);
  }
}

void Compiler::compile_variable(const ast::Variable &variable) {
  variable.visit(
      [this](const ast::Identifier &identifier) {
        emit(emit_get_variable(identifier));
      },
      [this](const ast::IndexExpression &index) {
        compile_variable(index.get_base());
        compile_expression(index.get_index());
        emit(OpGetIndex{});
      }
  );
}

void Compiler::compile_literal(const ast::Literal &literal) {
  literal.visit(
      [this](const ast::Nil &) { emit(OpConstant{make_constant({})}); },
      [this](const ast::Array &array) {
        const auto &elements = array.get_elements();
        for (const auto &elem : elements) {
          compile_expression(elem);
        }
        emit(OpMakeArray{elements.size()});
      },
      [this](const ast::String &string) {
        emit(
            OpConstant{
                make_constant(runtime::Value{std::string{string.get_value()}})
            }
        );
      },
      [this](const auto &literal_value) {
        emit(
            OpConstant{make_constant(
                runtime::Value{runtime::Primitive{literal_value.get_value()}}
            )}
        );
      }
  );
}

void Compiler::compile_statement(const ast::Statement &stmt) {
  stmt.visit(
      [this](const ast::Declaration &decl) { compile_declaration(decl); },
      [this](const ast::ForLoop &loop) { compile_for_loop(loop); },
      [this](const ast::FunctionCall &call) {
        compile_function_call_statement(call);
      },
      [this](const ast::IfStatement &if_stmt) {
        compile_if_statement(if_stmt);
      },
      [this](const ast::NameAssignment &assign) {
        compile_name_assignment(assign);
      },
      [this](const ast::NamedFunction &func) { compile_named_function(func); },
      [this](const ast::OperatorAssignment &assign) {
        compile_operator_assignment(assign);
      },
      [this](const ast::RangeForLoop &loop) { compile_range_for_loop(loop); },
      [this](const ast::While &loop) { compile_while_loop(loop); }
  );
}

void Compiler::compile_declaration(const ast::Declaration &decl) {
  const auto &names = decl.get_names();
  if (names.size() == 1) {
    if (decl.get_expression()) {
      compile_expression(*decl.get_expression());
    } else {
      emit(OpConstant{make_constant({})});
    }

    const auto &name = names.front();
    add_local(name);
    return;
  }
  if (!decl.get_expression()) {
    for (const auto &ident : names) {
      emit(OpConstant{make_constant({})});
      add_local(ident);
    }
    return;
  }

  compile_expression(*decl.get_expression());

  ast::Identifier hidden_name{
      "_destruct_" + std::to_string(scope_depth) + "_" +
      std::to_string(locals.size())
  };

  std::size_t hidden_name_index = -1UZ;
  hidden_name_index = add_local(hidden_name);

  for (std::size_t i = 0; i < names.size(); ++i) {
    emit(OpGetLocal{hidden_name_index});

    emit(
        OpConstant{make_constant(
            runtime::Value{runtime::Primitive{static_cast<std::int64_t>(i)}}
        )}
    );
    emit(OpGetIndex{});

    const auto &name = names[i];
    add_local(name);
  }
}

void Compiler::compile_for_loop(const ast::ForLoop &loop) {
  break_jumps_stack.emplace_back();
  continue_jumps_stack.emplace_back();
  begin_scope();

  compile_expression(loop.get_collection());
  const auto coll_idx = add_local("_for_collection");

  emit(emit_get_variable(ast::Identifier{"len"}));
  emit(OpGetLocal{coll_idx});
  emit(OpCall{1});
  const auto len_idx = add_local("_for_len");

  emit(OpConstant{make_constant(runtime::Value{runtime::Primitive{0L}})});
  const auto index_idx = add_local("_for_index");

  std::size_t loop_start = current_instruction_offset();

  emit(OpGetLocal{index_idx});
  emit(OpGetLocal{len_idx});
  emit(OpLess{});

  const std::size_t exit_jump = emit_jump(OpTest{});
  emit(OpPop{});

  // Snapshot locals before the inner scope so continue pops everything from the
  // inner scope (loop variable + body locals) without mutating `locals`.
  loop_body_locals_snapshot.push_back(locals.size());

  begin_scope();
  emit(OpGetLocal{coll_idx});
  emit(OpGetLocal{index_idx});
  emit(OpGetIndex{});
  add_local(loop.get_variable());

  compile_block(loop.get_body());

  end_scope();
  loop_body_locals_snapshot.pop_back();

  // continue target: after end_scope(), before the increment block.
  const auto continue_target = current_instruction_offset();
  loop_continues_stack.push_back(continue_target);

  emit(OpGetLocal{index_idx});
  emit(
      OpConstant{make_constant(
          runtime::Value{runtime::Primitive{static_cast<std::int64_t>(1)}}
      )}
  );
  emit(OpAdd{});
  emit(OpSetLocal{index_idx});

  emit_loop(loop_start);

  patch_jump_here(exit_jump);
  emit(OpPop{});

  end_scope();

  for (auto jump : break_jumps_stack.back()) {
    patch_jump_here(jump);
  }
  break_jumps_stack.pop_back();

  // Patch continue jumps to the continue target (forward jump — already past).
  for (auto jump : continue_jumps_stack.back()) {
    match::match(
        current_chunk().code[jump],
        [&](OpJump &j) { j.offset = continue_target - jump - 1; },
        [](auto &) {
          throw std::runtime_error("Expected OpJump at continue site");
        }
    );
  }
  continue_jumps_stack.pop_back();
  loop_continues_stack.pop_back();
}

void Compiler::compile_function_call_statement(const ast::FunctionCall &call) {
  compile_variable(ast::Variable{ast::Identifier{call.get_name().get_name()}});

  for (const auto &arg : call.get_arguments()) {
    compile_expression(arg);
  }

  emit(OpCall{call.get_arguments().size()});
  emit(OpPop{});
}

void Compiler::compile_if_statement(const ast::IfStatement &if_stmt) {
  std::vector<std::size_t> jump_to_ends;

  const auto &base_if = if_stmt.get_base_if();
  compile_expression(base_if.get_condition());
  std::size_t then_jump = emit_jump(OpTest{});

  emit(OpPop{});

  compile_block(base_if.get_block());
  jump_to_ends.push_back(emit_jump(OpJump{}));

  patch_jump_here(then_jump);
  emit(OpPop{});

  for (const auto &elif : if_stmt.get_elseif().get_elseifs()) {
    compile_expression(elif.get_condition());
    std::size_t elif_jump = emit_jump(OpTest{});

    emit(OpPop{});
    compile_block(elif.get_block());
    jump_to_ends.push_back(emit_jump(OpJump{}));

    patch_jump_here(elif_jump);
    emit(OpPop{});
  }

  if (auto else_block = if_stmt.get_else_block(); else_block) {
    compile_block(else_block->get());
  }

  for (auto jump : jump_to_ends) {
    patch_jump_here(jump);
  }
}

void Compiler::compile_name_assignment(const ast::NameAssignment &assign) {
  const auto &names = assign.get_names();
  compile_expression(assign.get_expression());

  if (names.size() == 1) {
    emit(emit_set_variable(names.front()));
    return;
  }

  ast::Identifier hidden_name{
      "_destruct_assign_" + std::to_string(scope_depth) + "_" +
      std::to_string(locals.size())
  };

  std::size_t hidden_name_idx = -1UZ;
  hidden_name_idx = add_local(hidden_name);

  for (std::size_t i = 0; i < names.size(); ++i) {
    emit(OpGetLocal{hidden_name_idx});

    emit(
        OpConstant{make_constant(
            runtime::Value{runtime::Primitive{static_cast<std::int64_t>(i)}}
        )}
    );
    emit(OpGetIndex{});

    emit_set_variable(names[i]);
  }
}

void Compiler::compile_named_function(const ast::NamedFunction &func) {
  const auto &name = func.get_name();
  bool is_local = true;

  if (is_local) {
    emit(OpConstant{make_constant({})});
    add_local(name);
  }

  const auto new_chunk_id = push_context();

  const auto &args = func.get_body().get_parameters();
  for (const auto &arg : args) {
    add_local(arg);
  }

  compile_block(func.get_body().get_block());
  emit(OpConstant{make_constant({})});
  emit(OpReturn{});

  std::vector<UpvalueInfo> upvalue_infos;
  upvalue_infos.reserve(current_upvalues.size());
  for (const auto &uv : current_upvalues) {
    upvalue_infos.emplace_back(uv.is_local, uv.index);
  }

  pop_context();

  std::size_t id = make_constant(
      runtime::Function{runtime::BytecodeFunctionId{
          .id = new_chunk_id,
          .name = name.get_name(),
          .arity = args.size(),
          .upvalues = std::vector<runtime::Ref>(),
          .curried_args = std::vector<runtime::Ref>()
      }}
  );

  if (upvalue_infos.empty()) {
    emit(OpConstant{id});
  } else {
    emit(OpClosure{.function_index = id, .upvalues = std::move(upvalue_infos)});
  }

  if (is_local) {
    emit(OpMutateLocal{static_cast<std::size_t>(locals.size() - 1)});
  }
}

void Compiler::compile_operator_assignment(
    const ast::OperatorAssignment &assign
) {
  assign.get_variable().visit(
      [&](const ast::Identifier &id) {
        if (assign.get_operator() != ast::AssignmentOperator::Assign) {
          emit(emit_get_variable(id));
        }

        compile_expression(assign.get_expression());

        switch (assign.get_operator()) {
        case ast::AssignmentOperator::Assign:
          break;
        case ast::AssignmentOperator::Plus:
          emit(OpAdd{});
          break;
        case ast::AssignmentOperator::Minus:
          emit(OpSubtract{});
          break;
        case ast::AssignmentOperator::Multiply:
          emit(OpMultiply{});
          break;
        case ast::AssignmentOperator::Divide:
          emit(OpDivide{});
          break;
        case ast::AssignmentOperator::Modulo:
          emit(OpModulo{});
          break;
        case ast::AssignmentOperator::Power:
          break; // TODO
        }

        emit(emit_set_variable(id));
      },
      [&](const ast::IndexExpression &idx) {
        if (assign.get_operator() != ast::AssignmentOperator::Assign) {
          compile_variable(idx.get_base());
          compile_expression(idx.get_index());
          compile_variable(idx.get_base());
          compile_expression(idx.get_index());
          emit(OpGetIndex{});

          compile_expression(assign.get_expression());

          switch (assign.get_operator()) {
          case ast::AssignmentOperator::Plus:
            emit(OpAdd{});
            break;
          case ast::AssignmentOperator::Minus:
            emit(OpSubtract{});
            break;
          case ast::AssignmentOperator::Multiply:
            emit(OpMultiply{});
            break;
          case ast::AssignmentOperator::Divide:
            emit(OpDivide{});
            break;
          case ast::AssignmentOperator::Modulo:
            emit(OpModulo{});
            break;
          case ast::AssignmentOperator::Power:
            break;
          default:
            std::unreachable();
          }

          emit(OpSetIndex{});
        } else {
          compile_variable(idx.get_base());
          compile_expression(idx.get_index());
          compile_expression(assign.get_expression());
          emit(OpSetIndex{});
        }
      }
  );
}

void Compiler::compile_range_for_loop(const ast::RangeForLoop &loop) {
  break_jumps_stack.emplace_back();
  continue_jumps_stack.emplace_back();
  begin_scope();

  compile_expression(loop.get_start());
  const auto current_idx = add_local("_range_current");

  compile_expression(loop.get_end());
  const auto end_idx = add_local("_range_end");

  if (loop.get_step()) {
    compile_expression(*loop.get_step());
  } else {
    emit(OpConstant{make_constant(runtime::Value{runtime::Primitive{1L}})});
  }
  const auto step_idx = add_local("_range_step");

  const auto loop_start = current_instruction_offset();

  emit(OpGetLocal{current_idx});
  emit(OpGetLocal{end_idx});

  if (loop.get_range_type() == ast::RangeOperator::Inclusive) {
    emit(OpLessEqual{});
  } else {
    emit(OpLess{});
  }

  const auto exit_jump = emit_jump(OpTest{});
  emit(OpPop{});

  loop_body_locals_snapshot.push_back(locals.size());

  begin_scope();
  emit(OpGetLocal{current_idx});
  add_local(loop.get_variable());

  compile_block(loop.get_body());

  end_scope();
  loop_body_locals_snapshot.pop_back();

  // continue target: after end_scope(), before the increment block.
  const auto continue_target = current_instruction_offset();

  emit(OpGetLocal{current_idx});
  emit(OpGetLocal{step_idx});
  emit(OpAdd{});
  emit(OpSetLocal{current_idx});

  emit_loop(loop_start);

  patch_jump_here(exit_jump);
  emit(OpPop{});

  end_scope();

  for (auto jump : break_jumps_stack.back()) {
    patch_jump_here(jump);
  }
  break_jumps_stack.pop_back();

  for (auto jump : continue_jumps_stack.back()) {
    patch_jump(jump, continue_target);
  }
  continue_jumps_stack.pop_back();
}

void Compiler::compile_while_loop(const ast::While &loop) {
  break_jumps_stack.emplace_back();
  continue_jumps_stack.emplace_back();
  const auto loop_start = current_instruction_offset();
  // `continue` in a while loop jumps back to the condition check.

  compile_expression(loop.get_condition());

  const auto exit_jump = emit_jump(OpTest{});

  emit(OpPop{});

  // Snapshot locals before the body scope so continue can compute how many
  // body-scope locals to pop.
  loop_body_locals_snapshot.push_back(locals.size());
  compile_block(loop.get_body());
  loop_body_locals_snapshot.pop_back();

  emit_loop(loop_start);

  patch_jump_here(exit_jump);

  emit(OpPop{});

  for (auto jump : break_jumps_stack.back()) {
    patch_jump_here(jump);
  }
  break_jumps_stack.pop_back();

  for (auto jump : continue_jumps_stack.back()) {
    current_chunk().code[jump] = OpJump{loop_start};
  }
  continue_jumps_stack.pop_back();
}

void Compiler::compile_last_statement(const ast::LastStatement &stmt) {
  stmt.visit(
      [this](const ast::ReturnStatement &ret) {
        compile_return_statement(ret);
      },
      [this](const ast::BreakStatement &brk) { compile_break_statement(brk); },
      [this](const ast::ContinueStatement &cont) {
        compile_continue_statement(cont);
      }
  );
}

void Compiler::compile_return_statement(const ast::ReturnStatement &ret) {
  if (ret.get_expression()) {
    compile_expression(*ret.get_expression());
  } else {
    emit(OpConstant{make_constant({})});
  }
  emit(OpReturn{});
}

void Compiler::compile_break_statement(const ast::BreakStatement & /* brk */) {
  if (break_jumps_stack.empty()) {
    throw std::runtime_error("Break statement outside of loop");
  }
  break_jumps_stack.back().push_back(emit_jump(OpJump{}));
}

void Compiler::
    compile_continue_statement(const ast::ContinueStatement & /* cont */) {
  if (continue_jumps_stack.empty()) {
    throw std::runtime_error("Continue statement outside of loop");
  }

  const auto body_locals = locals.size() - loop_body_locals_snapshot.back();
  for (auto i = 0UZ; i < body_locals; ++i) {
    emit(OpPop{});
  }
  continue_jumps_stack.back().push_back(emit_jump(OpJump{}));
}

} // namespace l3::compiler
