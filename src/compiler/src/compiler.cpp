module l3.compiler;

import std;
import l3.ast;
import l3.ast.printer;
import l3.bytecode;
import l3.runtime;
import utils;
import :peephole;

namespace l3::compiler {

using namespace l3::bytecode;

namespace {

class LocationScope {
  std::vector<location::Location> *stack;

public:
  LocationScope(const LocationScope &) = delete;
  LocationScope(LocationScope &&) = default;
  LocationScope &operator=(const LocationScope &) = delete;
  LocationScope &operator=(LocationScope &&) = delete;
  LocationScope(std::vector<location::Location> &stack, location::Location loc)
      : stack(&stack) {
    stack.push_back(loc);
  }

  ~LocationScope() { stack->pop_back(); }
};

} // namespace

Compiler::Compiler(ProgramBytecode &program) : program(program) {}

void Compiler::compile(const ast::Program &ast_program) {
  contexts.emplace_back(std::vector<Local>{}, std::vector<Upvalue>{}, 0);
  program.chunks.emplace_back();
  compile_statements(ast_program.get_statements());
  if (const auto last = ast_program.get_last_statement(); last) {
    throw CompileError("Unexpected last statement in top-level scope");
  }
  emit(OpConstant{make_constant()});
  emit(OpReturn{});
  optimize(program);
  deduplicate_constants();
}

Chunk &Compiler::current_chunk() {
  return program.chunks[contexts.back().chunk_id];
}
std::size_t Compiler::last_instruction_offset() {
  return current_instruction_offset() - 1;
}
std::size_t Compiler::current_instruction_offset() {
  return current_chunk().code.size();
}

auto &&Compiler::locals(this auto &&self) {
  return self.contexts.back().locals;
}
auto &&Compiler::upvalues(this auto &&self) {
  return self.contexts.back().upvalues;
}
auto &&Compiler::scope_depth(this auto &&self) {
  return self.contexts.back().scope_depth;
}

const location::Location &Compiler::current_location() const {
  if (location_stack.empty()) {
    static const location::Location empty{};
    return empty;
  }
  return location_stack.back();
}

std::size_t Compiler::push_context() {
  const auto chunk_id = program.chunks.size();
  contexts.emplace_back(std::vector<Local>{}, std::vector<Upvalue>{}, chunk_id);
  program.chunks.emplace_back();
  return chunk_id;
}

void Compiler::pop_context() { contexts.pop_back(); }

void Compiler::begin_scope() { scope_depth()++; }

void Compiler::end_scope(bool emit_pop) {
  scope_depth()--;
  std::size_t count = 0;
  while (!locals().empty() && locals().back().depth > scope_depth()) {
    count++;
    locals().pop_back();
  }
  if (emit_pop) {
    emit(OpPop{.count = count});
  }
}

std::size_t Compiler::add_local(const ast::Identifier &name) {
  auto index = locals().size();
  locals().emplace_back(name, scope_depth());
  return index;
}

namespace {

std::optional<std::size_t>
resolve_in_context(const ast::Identifier &name, const Context &ctx) {
  for (const auto &[i, local] :
       std::views::reverse(utils::ranges::enumerate(ctx.locals))) {
    if (local.name == name) {
      return i;
    }
  }
  return std::nullopt;
}

} // namespace

std::optional<std::size_t>
Compiler::resolve_local(const ast::Identifier &name) const {
  return resolve_in_context(name, contexts.back());
}

std::optional<std::size_t>
Compiler::resolve_upvalue(const ast::Identifier &name) {
  const auto add_upvalue_to_context =
      [](bool is_local, std::size_t index, Context &ctx) -> std::size_t {
    // Add upvalue to upvalues vector if not already present
    for (const auto &[i, uv] : utils::ranges::enumerate(ctx.upvalues)) {
      if (uv.is_local == is_local && uv.index == index) {
        return i;
      }
    }
    const auto upvalue_idx = ctx.upvalues.size();
    ctx.upvalues.emplace_back(is_local, index);
    return upvalue_idx;
  };

  // iterate over contexts except the last in reverse order
  for (const auto &[i, ctx] : std::views::drop(
           std::views::reverse(utils::ranges::enumerate(contexts)), 1
       )) {
    auto index = resolve_in_context(name, ctx);
    if (!index) {
      continue;
    }

    auto result = add_upvalue_to_context(true, *index, contexts[i + 1]);

    for (std::size_t j = i + 2; j < contexts.size(); ++j) {
      result = add_upvalue_to_context(false, result, contexts[j]);
    }

    return result;
  }
  return std::nullopt;
}

Compiler::ResolvedVariable
Compiler::resolve_variable(const ast::Identifier &identifier) {
  if (auto local_arg = resolve_local(identifier)) {
    return {.type = Compiler::VariableType::Local, .index = *local_arg};
  }

  if (auto upvalue_arg = resolve_upvalue(identifier)) {
    return {.type = Compiler::VariableType::Upvalue, .index = *upvalue_arg};
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

void Compiler::emit(const Instruction &instruction) {
  current_chunk().write(instruction, current_location());
}

void Compiler::emit(
    const Instruction &instruction, const location::Location &location
) {
  current_chunk().write(instruction, location);
}

std::size_t Compiler::make_constant(runtime::Value &&value) {
  auto index = program.constants.size();
  program.constants.emplace_back(std::move(value));
  return index;
}

void Compiler::deduplicate_constants() {
  std::vector<std::size_t> index_map(program.constants.size(), 0UZ);
  std::vector<runtime::GCValue> deduped_constants;

  for (const auto &[old_index, constant] :
       utils::ranges::enumerate(program.constants)) {
    const auto &value = constant.get_value();

    auto it =
        std::ranges::find_if(deduped_constants, [&value](const auto &other) {
          return value.compare(other.get_value()) ==
                 std::partial_ordering::equivalent;
        });

    if (it != deduped_constants.end()) {
      index_map[old_index] = static_cast<std::size_t>(
          std::distance(deduped_constants.begin(), it)
      );
    } else {
      index_map[old_index] = deduped_constants.size();
      deduped_constants.emplace_back(std::move(constant));
    }
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
          [](const auto &) {}
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
      [=](OpJumpIf &jump) { jump.offset = target; },
      [](const auto &) {
        throw CompileError("Attempting to patch a non-jump instruction");
      }
  );
}

void Compiler::patch_jump_here(std::size_t jump_offset) {
  patch_jump(jump_offset, last_instruction_offset() + 1);
}

std::size_t Compiler::emit_jump(const Instruction &instruction) {
  current_chunk().write(instruction, current_location());
  return last_instruction_offset();
}

void Compiler::compile_statements(std::ranges::input_range auto &statements) {
  // hoist function declarations
  for (const auto &stmt : statements) {
    LocationScope scope(location_stack, stmt.get_location());
    stmt.visit(
        [this](const ast::NamedFunction &func) {
          emit(OpConstant{make_constant()});
          add_local(func.get_name());
        },
        [](const auto &) {}
    );
  }

  for (const auto &stmt : statements) {
    compile_statement(stmt);
  }
}

void Compiler::compile_block(const ast::Block &block) {
  begin_scope();
  compile_statements(block.get_statements());
  if (const auto last = block.get_last_statement()) {
    compile_last_statement(last->get());
    end_scope(false);
  } else {
    end_scope(true);
  }
}

void Compiler::compile_expression(const ast::Expression &expr) {
  LocationScope scope(location_stack, expr.get_location());
  bool prev_in_expression = contexts.back().is_in_expression;
  contexts.back().is_in_expression = true;
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
  contexts.back().is_in_expression = prev_in_expression;
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
  case ast::BinaryOperator::Power:
    emit(OpPower{});
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
    std::size_t jump = emit_jump(OpJumpIf{.keep_jump = true});
    compile_expression(logical.get_rhs());
    patch_jump_here(jump);
  } break;
  case ast::LogicalOperator::Or: {
    std::size_t jump = emit_jump(OpJumpIf{.expected = true, .keep_jump = true});
    compile_expression(logical.get_rhs());
    patch_jump_here(jump);
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
      jumps.push_back(
          emit_jump(OpJumpIf{.expected = false, .keep_jump = true})
      );
    }
  }

  for (auto jump : jumps) {
    patch_jump_here(jump);
  }
}

void Compiler::compile_anonymous_function(const ast::AnonymousFunction &func) {
  auto [used_upvalues, new_chunk_id] = compile_function_body(func.get_body());

  std::size_t id = make_constant(
      runtime::Function{runtime::BytecodeFunction{
          .id = new_chunk_id,
          .name = "<anonymous>",
          .arity = func.get_arity(),
          .curried_args = {},
          .captured_upvalue_refs = {}
      }}
  );

  if (used_upvalues.empty()) {
    emit(OpConstant{id});
  } else {
    emit(OpClosure{.function_index = id, .upvalues = std::move(used_upvalues)});
  }
}

void Compiler::compile_function_call(const ast::FunctionCall &call) {
  compile_variable(ast::Variable{ast::Identifier{call.get_name()}});
  const auto &args = call.get_arguments();
  for (const auto &arg : args) {
    compile_expression(arg);
  }
  emit(OpCall{.arg_count = args.size()});
}

void Compiler::compile_if_expression(const ast::IfExpression &if_expr) {
  std::vector<std::size_t> jumps_to_end;

  const auto &base_if = if_expr.get_base_if();

  compile_expression(base_if.get_condition());
  std::size_t negative_jump = emit_jump(OpJumpIf{});

  compile_block(base_if.get_block());

  jumps_to_end.push_back(emit_jump(OpJump{}));

  patch_jump_here(negative_jump);

  for (const auto &elif : if_expr.get_elseif().get_elseifs()) {
    compile_expression(elif.get_condition());
    std::size_t elif_jump = emit_jump(OpJumpIf{});

    compile_block(elif.get_block());

    jumps_to_end.push_back(emit_jump(OpJump{}));

    patch_jump_here(elif_jump);
  }

  compile_block(if_expr.get_else_block());

  for (auto jump : jumps_to_end) {
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
      [this](const ast::Nil &) { emit(OpConstant{make_constant()}); },
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
  LocationScope scope(location_stack, stmt.get_location());
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
      emit(OpConstant{make_constant()});
    }

    add_local(names.front());
    return;
  }
  if (!decl.get_expression()) {
    for (const auto &ident : names) {
      emit(OpConstant{make_constant()});
      add_local(ident);
    }
    return;
  }

  compile_expression(*decl.get_expression());

  ast::Identifier hidden_name{
      "_destruct_" + std::to_string(scope_depth()) + "_" +
      std::to_string(locals().size())
  };

  std::size_t hidden_name_index = add_local(hidden_name);

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
  loop_contexts.emplace_back();
  begin_scope();

  compile_expression(loop.get_collection());
  const auto coll_idx = add_local("_for_collection");

  emit(emit_get_variable(ast::Identifier{"len"}));
  emit(OpGetLocal{coll_idx});
  emit(OpCall{.arg_count = 1, .keep_return_value = true});
  const auto len_idx = add_local("_for_len");

  emit(OpConstant{make_constant(runtime::Value{runtime::Primitive{-1L}})});
  const auto index_idx = add_local("_for_index");

  const auto control_jump = emit_jump(OpJump{});

  const auto body_offset = current_instruction_offset();

  loop_contexts.back().body_locals_snapshot = locals().size();

  begin_scope();
  emit(OpGetLocal{coll_idx});
  emit(OpGetLocal{index_idx});
  emit(OpGetIndex{});
  add_local(loop.get_variable());

  compile_block(loop.get_body());

  end_scope();

  const auto control_offset = current_instruction_offset();

  emit(
      OpForLoop{
          .control_index = index_idx,
          .limit_index = len_idx,
          .body_offset = body_offset,
          .inclusive = false,
          .step_index = std::nullopt
      }
  );

  patch_jump(control_jump, control_offset);

  end_scope();

  for (auto jump : loop_contexts.back().break_jumps) {
    patch_jump_here(jump);
  }
  for (auto jump : loop_contexts.back().continue_jumps) {
    patch_jump(jump, control_offset);
  }
  loop_contexts.pop_back();
}

void Compiler::compile_function_call_statement(const ast::FunctionCall &call) {
  compile_variable(ast::Variable{ast::Identifier{call.get_name().get_name()}});

  for (const auto &arg : call.get_arguments()) {
    compile_expression(arg);
  }

  emit(
      OpCall{
          .arg_count = call.get_arguments().size(), .keep_return_value = false
      }
  );
}

void Compiler::compile_if_statement(const ast::IfStatement &if_stmt) {
  std::vector<std::size_t> jump_to_ends;

  const auto &base_if = if_stmt.get_base_if();
  compile_expression(base_if.get_condition());
  std::size_t negative_jump = emit_jump(OpJumpIf{});

  compile_block(base_if.get_block());
  jump_to_ends.push_back(emit_jump(OpJump{}));

  patch_jump_here(negative_jump);

  for (const auto &elif : if_stmt.get_elseif().get_elseifs()) {
    compile_expression(elif.get_condition());
    std::size_t negative_jump = emit_jump(OpJumpIf{});

    compile_block(elif.get_block());
    jump_to_ends.push_back(emit_jump(OpJump{}));

    patch_jump_here(negative_jump);
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
      "_destruct_assign_" + std::to_string(scope_depth()) + "_" +
      std::to_string(locals().size())
  };

  std::size_t hidden_name_idx = add_local(hidden_name);

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

Compiler::CompiledFunctionBody
Compiler::compile_function_body(const ast::FunctionBody &body) {
  LocationScope scope(location_stack, body.get_location());
  const auto chunk_id = push_context();

  const auto &args = body.get_parameters();
  for (const auto &arg : args) {
    add_local(arg);
  }

  compile_block(body.get_block());

  if (!body.get_block().get_last_statement()) {
    emit(OpConstant{make_constant()});
    emit(OpReturn{});
  }

  auto used_upvalues = std::move(upvalues());
  pop_context();
  return {.upvalues = std::move(used_upvalues), .chunk_id = chunk_id};
}

void Compiler::compile_named_function(const ast::NamedFunction &func) {
  const auto &name = func.get_name();

  auto local_opt = resolve_local(name);
  if (!local_opt) {
    throw CompileError(
        std::format(
            "Expected function {} to be already defined", name.get_name()
        )
    );
  }

  auto [used_upvalues, chunk_id] = compile_function_body(func.get_body());

  const auto constant = make_constant(
      runtime::Function{runtime::BytecodeFunction{
          .id = chunk_id,
          .name = name.get_name(),
          .arity = func.get_arity(),
          .curried_args = {},
          .captured_upvalue_refs = {}
      }}
  );

  if (used_upvalues.empty()) {
    emit(OpConstant{constant});
  } else {
    emit(
        OpClosure{
            .function_index = constant, .upvalues = std::move(used_upvalues)
        }
    );
  }

  emit(OpSetLocal{*local_opt});
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
          emit(OpDuplicate{1});
          emit(OpDuplicate{1});
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
  loop_contexts.emplace_back();
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

  emit(OpGetLocal{current_idx});
  emit(OpGetLocal{step_idx});
  emit(OpSubtract{});
  emit(OpSetLocal{current_idx});

  const auto control_jump = emit_jump(OpJump{});

  const auto body_offset = current_instruction_offset();

  loop_contexts.back().body_locals_snapshot = locals().size();

  begin_scope();
  emit(OpGetLocal{current_idx});
  add_local(loop.get_variable());

  compile_block(loop.get_body());

  end_scope();

  const auto control_offset = current_instruction_offset();

  emit(
      OpForLoop{
          .control_index = current_idx,
          .limit_index = end_idx,
          .body_offset = body_offset,
          .inclusive = loop.get_range_type() == ast::RangeOperator::Inclusive,
          .step_index = step_idx
      }
  );

  patch_jump(control_jump, control_offset);

  end_scope();

  for (auto jump : loop_contexts.back().break_jumps) {
    patch_jump_here(jump);
  }
  for (auto jump : loop_contexts.back().continue_jumps) {
    patch_jump(jump, control_offset);
  }
  loop_contexts.pop_back();
}

void Compiler::compile_while_loop(const ast::While &loop) {
  loop_contexts.emplace_back();
  const auto loop_start = current_instruction_offset();

  compile_expression(loop.get_condition());

  const auto exit_jump = emit_jump(OpJumpIf{});

  loop_contexts.back().body_locals_snapshot = locals().size();
  compile_block(loop.get_body());

  emit_loop(loop_start);

  patch_jump_here(exit_jump);

  for (auto jump : loop_contexts.back().break_jumps) {
    patch_jump_here(jump);
  }
  for (auto jump : loop_contexts.back().continue_jumps) {
    patch_jump(jump, loop_start);
  }
  loop_contexts.pop_back();
}

void Compiler::compile_last_statement(const ast::LastStatement &stmt) {
  LocationScope scope(location_stack, stmt.get_location());
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
  bool was_in_expression = contexts.back().is_in_expression;
  if (const auto &expr = ret.get_expression(); expr) {
    compile_expression(*expr);
  } else {
    emit(OpConstant{make_constant()});
  }
  if (!was_in_expression) {
    emit(OpReturn{});
  }
}

void Compiler::compile_break_statement(const ast::BreakStatement & /* brk */) {
  if (loop_contexts.empty()) {
    throw CompileError("Break statement outside of loop");
  }
  loop_contexts.back().break_jumps.push_back(emit_jump(OpJump{}));
}

void Compiler::
    compile_continue_statement(const ast::ContinueStatement & /* cont */) {
  if (loop_contexts.empty()) {
    throw CompileError("Continue statement outside of loop");
  }

  const auto body_locals =
      locals().size() - loop_contexts.back().body_locals_snapshot;
  emit(OpPop{.count = body_locals});
  loop_contexts.back().continue_jumps.push_back(emit_jump(OpJump{}));
}

} // namespace l3::compiler
