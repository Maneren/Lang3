module l3.compiler;

import std;
import l3.ast;
import l3.bytecode;
import l3.runtime;
import utils;

namespace l3::compiler {

Compiler::Compiler(std::vector<bytecode::Chunk> &chunks) : chunks(chunks) {}

void Compiler::compile(const ast::Program &program) {
  chunks.emplace_back();
  current_chunk_id = chunks.size() - 1;
  for (const auto &stmt : program.get_statements()) {
    compile_statement(stmt);
  }
  if (auto last = program.get_last_statement(); last) {
    compile_last_statement(last->get());
  }
  emit(bytecode::OpConstant{make_constant(runtime::Value{runtime::Nil{}})});
  emit(bytecode::OpReturn{});
}

bytecode::Chunk &Compiler::current_chunk() { return chunks[current_chunk_id]; }
std::size_t Compiler::last_instruction_offset() {
  return current_instruction_offset() - 1;
}
std::size_t Compiler::current_instruction_offset() {
  return current_chunk().code.size();
}

void Compiler::push_context(std::size_t new_chunk_id) {
  contexts.push_back(
      {.chunk_id = current_chunk_id,
       .locals = std::move(locals),
       .scope_depth = scope_depth,
       .upvalues = std::move(current_upvalues)}
  );
  current_chunk_id = new_chunk_id;
  locals.clear();
  scope_depth = 0;
  current_upvalues.clear();
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
    emit(bytecode::OpPop{});
    locals.pop_back();
  }
}

int Compiler::resolve_local(const std::string &name) {
  for (int i = static_cast<int>(locals.size()) - 1; i >= 0; i--) {
    if (locals[static_cast<std::size_t>(i)].name == name) {
      return i;
    }
  }
  return -1;
}

int Compiler::resolve_local_in_context(
    const std::string &name, const Context &ctx
) {
  for (int i = static_cast<int>(ctx.locals.size()) - 1; i >= 0; i--) {
    if (ctx.locals[static_cast<std::size_t>(i)].name == name) {
      return i;
    }
  }
  return -1;
}

int Compiler::add_upvalue(
    std::vector<Upvalue> &upvalues, bool is_local, std::size_t index
) {
  for (std::size_t i = 0; i < upvalues.size(); i++) {
    if (upvalues[i].is_local == is_local && upvalues[i].index == index) {
      return static_cast<int>(i);
    }
  }
  upvalues.push_back({.is_local = is_local, .index = index});
  return static_cast<int>(upvalues.size() - 1);
}

int Compiler::resolve_upvalue(const std::string &name, int context_index) {
  if (context_index < 0) {
    return -1;
  }

  int local = resolve_local_in_context(
      name, contexts[static_cast<std::size_t>(context_index)]
  );
  if (local != -1) {
    int upvalue_idx = -1;
    if (static_cast<std::size_t>(context_index + 1) == contexts.size()) {
      upvalue_idx =
          add_upvalue(current_upvalues, true, static_cast<std::size_t>(local));
    } else {
      upvalue_idx = add_upvalue(
          contexts[static_cast<std::size_t>(context_index + 1)].upvalues,
          true,
          static_cast<std::size_t>(local)
      );
    }
    for (auto j = static_cast<std::size_t>(context_index + 2);
         j <= contexts.size();
         j++) {
      if (j == contexts.size()) {
        upvalue_idx = add_upvalue(
            current_upvalues, false, static_cast<std::size_t>(upvalue_idx)
        );
      } else {
        upvalue_idx = add_upvalue(
            contexts[j].upvalues, false, static_cast<std::size_t>(upvalue_idx)
        );
      }
    }
    return upvalue_idx;
  }

  return resolve_upvalue(name, context_index - 1);
}

void Compiler::emit(
    const bytecode::Instruction &instruction, std::size_t line
) {
  current_chunk().write(instruction, line);
}

std::size_t Compiler::make_constant(runtime::Value &&value) {
  return current_chunk().add_constant(std::move(value));
}

void Compiler::emit_loop(std::size_t loop_start) {
  emit(bytecode::OpJump{loop_start});
}

void Compiler::patch_jump(std::size_t jump_offset, std::size_t target) {
  match::match(
      current_chunk().code[jump_offset],
      [=](bytecode::OpJump &jump) { jump.offset = target; },
      [=](bytecode::OpJumpIfFalse &jump) { jump.offset = target; },
      [](auto &) {
        throw std::runtime_error("Attempting to patch a non-jump instruction");
      }
  );
}

void Compiler::patch_jump_here(std::size_t jump_offset) {
  patch_jump(jump_offset, last_instruction_offset() + 1);
}

std::size_t Compiler::emit_jump(const bytecode::Instruction &instruction) {
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
    emit(bytecode::OpAdd{});
    break;
  case ast::BinaryOperator::Minus:
    emit(bytecode::OpSubtract{});
    break;
  case ast::BinaryOperator::Multiply:
    emit(bytecode::OpMultiply{});
    break;
  case ast::BinaryOperator::Divide:
    emit(bytecode::OpDivide{});
    break;
  case ast::BinaryOperator::Modulo:
    emit(bytecode::OpModulo{});
    break;
  case ast::BinaryOperator::Power: /* emit(bytecode::OpPower{}); */
    break;
  }
}

void Compiler::compile_unary_expression(const ast::UnaryExpression &unary) {
  compile_expression(unary.get_expression());

  switch (unary.get_op()) {
  case ast::UnaryOperator::Minus:
    emit(bytecode::OpNegate{});
    break;
  case ast::UnaryOperator::Not:
    emit(bytecode::OpNot{});
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
    std::size_t jump = emit_jump(bytecode::OpJumpIfFalse{});
    emit(bytecode::OpPop{});
    compile_expression(logical.get_rhs());
    patch_jump_here(jump);
  } break;
  case ast::LogicalOperator::Or: { // Logical OR
    std::size_t jump_if_false = emit_jump(bytecode::OpJumpIfFalse{});
    std::size_t jump_to_end = emit_jump(bytecode::OpJump{});

    patch_jump_here(jump_if_false);
    emit(bytecode::OpPop{});
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
      emit(bytecode::OpEqual{});
      break;
    case ast::ComparisonOperator::NotEqual:
      emit(bytecode::OpNotEqual{});
      break;
    case ast::ComparisonOperator::Greater:
      emit(bytecode::OpGreater{});
      break;
    case ast::ComparisonOperator::GreaterEqual:
      emit(bytecode::OpGreaterEqual{});
      break;
    case ast::ComparisonOperator::Less:
      emit(bytecode::OpLess{});
      break;
    case ast::ComparisonOperator::LessEqual:
      emit(bytecode::OpLessEqual{});
      break;
    }

    if (i < comps.size() - 1) {
      jumps.push_back(emit_jump(bytecode::OpJumpIfFalse{}));
      emit(bytecode::OpPop{});
    }
  }

  for (auto jump : jumps) {
    patch_jump_here(jump);
  }
}

void Compiler::compile_anonymous_function(const ast::AnonymousFunction &func) {
  chunks.emplace_back();
  std::size_t new_chunk_id = chunks.size() - 1;
  push_context(new_chunk_id);

  const auto &args = func.get_body().get_parameters();
  for (const auto &arg : args) {
    locals.push_back({std::string{arg.get_name()}, scope_depth});
  }

  compile_block(func.get_body().get_block());
  emit(bytecode::OpConstant{make_constant(runtime::Value{runtime::Nil{}})});
  emit(bytecode::OpReturn{});

  std::vector<bytecode::UpvalueInfo> upvalue_infos;
  for (const auto &uv : current_upvalues) {
    upvalue_infos.push_back({uv.is_local, uv.index});
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
    emit(bytecode::OpConstant{id});
  } else {
    emit(
        bytecode::OpClosure{
            .function_index = id, .upvalues = std::move(upvalue_infos)
        }
    );
  }
}

void Compiler::compile_function_call(const ast::FunctionCall &call) {
  compile_variable(ast::Variable{ast::Identifier{call.get_name().get_name()}});
  for (const auto &arg : call.get_arguments()) {
    compile_expression(arg);
  }
  emit(bytecode::OpCall{call.get_arguments().size()});
}

void Compiler::compile_if_expression(const ast::IfExpression &if_expr) {
  std::vector<std::size_t> jump_to_ends;

  const auto &base_if = if_expr.get_base_if();

  compile_expression(base_if.get_condition());
  std::size_t then_jump = emit_jump(bytecode::OpJumpIfFalse{});
  emit(bytecode::OpPop{});

  compile_block(base_if.get_block());

  emit(bytecode::OpConstant{make_constant(runtime::Value{runtime::Nil{}})});
  jump_to_ends.push_back(emit_jump(bytecode::OpJump{}));

  patch_jump_here(then_jump);
  emit(bytecode::OpPop{});

  for (const auto &elif : if_expr.get_elseif().get_elseifs()) {
    compile_expression(elif.get_condition());
    std::size_t elif_jump = emit_jump(bytecode::OpJumpIfFalse{});

    emit(bytecode::OpPop{});
    compile_block(elif.get_block());
    emit(bytecode::OpConstant{make_constant(runtime::Value{runtime::Nil{}})});
    jump_to_ends.push_back(emit_jump(bytecode::OpJump{}));

    patch_jump_here(elif_jump);
    emit(bytecode::OpPop{});
  }

  compile_block(if_expr.get_else_block());
  emit(bytecode::OpConstant{make_constant(runtime::Value{runtime::Nil{}})});

  for (auto jump : jump_to_ends) {
    patch_jump_here(jump);
  }
}

void Compiler::compile_variable(const ast::Variable &variable) {
  variable.visit(
      [this](const ast::Identifier &identifier) {
        const std::string &name = identifier.get_name();
        int local_arg = resolve_local(name);
        if (local_arg != -1) {
          emit(bytecode::OpGetLocal{static_cast<std::size_t>(local_arg)});
        } else {
          int upvalue_arg =
              resolve_upvalue(name, static_cast<int>(contexts.size()) - 1);
          if (upvalue_arg != -1) {
            emit(bytecode::OpGetUpvalue{static_cast<std::size_t>(upvalue_arg)});
          } else {
            std::size_t name_index =
                make_constant(runtime::Value{std::string{name}});
            emit(bytecode::OpGetGlobal{name_index});
          }
        }
      },
      [this](const ast::IndexExpression &index) {
        compile_variable(index.get_base());
        compile_expression(index.get_index());
        emit(bytecode::OpGetIndex{});
      }
  );
}

void Compiler::compile_literal(const ast::Literal &literal) {
  literal.visit(
      [this](const ast::Nil &) {
        emit(
            bytecode::OpConstant{make_constant(runtime::Value{runtime::Nil{}})}
        );
      },
      [this](const ast::Array &array) {
        const auto &elements = array.get_elements();
        for (const auto &elem : elements) {
          compile_expression(elem);
        }
        emit(bytecode::OpMakeArray{elements.size()});
      },
      [this](const ast::String &string) {
        emit(
            bytecode::OpConstant{
                make_constant(runtime::Value{std::string{string.get_value()}})
            }
        );
      },
      [this](const auto &literal_value) {
        emit(
            bytecode::OpConstant{make_constant(
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
    const auto &name = names.front().get_name();
    std::size_t name_index = make_constant(runtime::Value{std::string{name}});

    if (decl.get_expression()) {
      compile_expression(*decl.get_expression());
    } else {
      emit(bytecode::OpConstant{make_constant(runtime::Value{runtime::Nil{}})});
    }

    if (scope_depth > 0) {
      locals.push_back({std::string{name}, scope_depth});
    } else {
      emit(bytecode::OpDefineGlobal{name_index});
    }
  } else {
    if (!decl.get_expression()) {
      for (const auto &ident : names) {
        const std::string &name{ident.get_name()};
        std::size_t name_index = make_constant(std::string{name});
        emit(bytecode::OpConstant{make_constant(runtime::Nil{})});
        if (scope_depth > 0) {
          locals.push_back({.name = std::string{name}, .depth = scope_depth});
        } else {
          emit(bytecode::OpDefineGlobal{name_index});
        }
      }
      return;
    }

    compile_expression(*decl.get_expression());

    std::string hidden_name = "_destruct_" + std::to_string(scope_depth) + "_" +
                              std::to_string(locals.size());

    if (scope_depth > 0) {
      locals.push_back({.name = hidden_name, .depth = scope_depth});
    } else {
      std::size_t hidden_idx =
          make_constant(runtime::Value{std::string{hidden_name}});
      emit(bytecode::OpDefineGlobal{hidden_idx});
    }

    for (std::size_t i = 0; i < names.size(); ++i) {
      if (scope_depth > 0) {
        emit(
            bytecode::OpGetLocal{
                static_cast<std::size_t>(resolve_local(hidden_name))
            }
        );
      } else {
        std::size_t hidden_idx =
            make_constant(runtime::Value{std::string{hidden_name}});
        emit(bytecode::OpGetGlobal{hidden_idx});
      }

      emit(
          bytecode::OpConstant{make_constant(
              runtime::Value{runtime::Primitive{static_cast<std::int64_t>(i)}}
          )}
      );
      emit(bytecode::OpGetIndex{});

      const auto &name = names[i].get_name();
      std::size_t name_index = make_constant(runtime::Value{std::string{name}});
      if (scope_depth > 0) {
        locals.push_back({.name = std::string{name}, .depth = scope_depth});
      } else {
        emit(bytecode::OpDefineGlobal{name_index});
      }
    }
  }
}

void Compiler::compile_for_loop(const ast::ForLoop &loop) {
  break_jumps_stack.emplace_back();
  continue_jumps_stack.emplace_back();
  begin_scope();

  compile_expression(loop.get_collection());
  locals.push_back({.name = "_for_collection", .depth = scope_depth});

  std::size_t len_name_index =
      make_constant(runtime::Value{std::string{"len"}});
  emit(bytecode::OpGetGlobal{len_name_index});
  emit(
      bytecode::OpGetLocal{
          static_cast<std::size_t>(resolve_local("_for_collection"))
      }
  );
  emit(bytecode::OpCall{1});
  locals.push_back({.name = "_for_len", .depth = scope_depth});

  emit(
      bytecode::OpConstant{make_constant(
          runtime::Value{runtime::Primitive{static_cast<std::int64_t>(0)}}
      )}
  );
  locals.push_back({.name = "_for_index", .depth = scope_depth});

  std::size_t loop_start = current_chunk().code.size();

  std::size_t index_idx = static_cast<std::size_t>(resolve_local("_for_index"));
  std::size_t len_idx = static_cast<std::size_t>(resolve_local("_for_len"));
  std::size_t coll_idx =
      static_cast<std::size_t>(resolve_local("_for_collection"));

  emit(bytecode::OpGetLocal{index_idx});
  emit(bytecode::OpGetLocal{len_idx});
  emit(bytecode::OpLess{});

  std::size_t exit_jump = emit_jump(bytecode::OpJumpIfFalse{});
  emit(bytecode::OpPop{});

  // Snapshot locals before the inner scope so continue pops everything from the
  // inner scope (loop variable + body locals) without mutating `locals`.
  loop_body_locals_snapshot.push_back(locals.size());

  begin_scope();
  emit(bytecode::OpGetLocal{coll_idx});
  emit(bytecode::OpGetLocal{index_idx});
  emit(bytecode::OpGetIndex{});
  locals.push_back(
      {.name = std::string{loop.get_variable().get_name()},
       .depth = scope_depth}
  );

  compile_block(loop.get_body());

  end_scope();
  loop_body_locals_snapshot.pop_back();

  // continue target: after end_scope(), before the increment block.
  std::size_t continue_target = current_chunk().code.size();
  loop_continues_stack.push_back(continue_target);

  emit(bytecode::OpGetLocal{index_idx});
  emit(
      bytecode::OpConstant{make_constant(
          runtime::Value{runtime::Primitive{static_cast<std::int64_t>(1)}}
      )}
  );
  emit(bytecode::OpAdd{});
  emit(bytecode::OpSetLocal{index_idx});

  emit_loop(loop_start);

  patch_jump_here(exit_jump);
  emit(bytecode::OpPop{});

  end_scope();

  for (auto jump : break_jumps_stack.back()) {
    patch_jump_here(jump);
  }
  break_jumps_stack.pop_back();

  // Patch continue jumps to the continue target (forward jump — already past).
  for (auto jump : continue_jumps_stack.back()) {
    match::match(
        current_chunk().code[jump],
        [&](bytecode::OpJump &j) { j.offset = continue_target - jump - 1; },
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

  emit(bytecode::OpCall{call.get_arguments().size()});
  emit(bytecode::OpPop{});
}

void Compiler::compile_if_statement(const ast::IfStatement &if_stmt) {
  std::vector<std::size_t> jump_to_ends;

  const auto &base_if = if_stmt.get_base_if();
  compile_expression(base_if.get_condition());
  std::size_t then_jump = emit_jump(bytecode::OpJumpIfFalse{});

  emit(bytecode::OpPop{});

  compile_block(base_if.get_block());
  jump_to_ends.push_back(emit_jump(bytecode::OpJump{}));

  patch_jump_here(then_jump);
  emit(bytecode::OpPop{});

  for (const auto &elif : if_stmt.get_elseif().get_elseifs()) {
    compile_expression(elif.get_condition());
    std::size_t elif_jump = emit_jump(bytecode::OpJumpIfFalse{});

    emit(bytecode::OpPop{});
    compile_block(elif.get_block());
    jump_to_ends.push_back(emit_jump(bytecode::OpJump{}));

    patch_jump_here(elif_jump);
    emit(bytecode::OpPop{});
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
  if (names.size() == 1) {
    compile_expression(assign.get_expression());
    std::string name{names.front().get_name()};
    int local_arg = resolve_local(name);
    if (local_arg != -1) {
      emit(bytecode::OpSetLocal{static_cast<std::size_t>(local_arg)});
    } else {
      int upvalue_arg =
          resolve_upvalue(name, static_cast<int>(contexts.size()) - 1);
      if (upvalue_arg != -1) {
        emit(bytecode::OpSetUpvalue{static_cast<std::size_t>(upvalue_arg)});
      } else {
        std::size_t name_index =
            make_constant(runtime::Value{std::string{name}});
        emit(bytecode::OpSetGlobal{name_index});
      }
    }
  } else {
    compile_expression(assign.get_expression());

    std::string hidden_name = "_destruct_assign_" +
                              std::to_string(scope_depth) + "_" +
                              std::to_string(locals.size());

    if (scope_depth > 0) {
      locals.push_back({hidden_name, scope_depth});
    } else {
      std::size_t hidden_idx =
          make_constant(runtime::Value{std::string{hidden_name}});
      emit(bytecode::OpDefineGlobal{hidden_idx});
    }

    for (std::size_t i = 0; i < names.size(); ++i) {
      if (scope_depth > 0) {
        emit(
            bytecode::OpGetLocal{
                static_cast<std::size_t>(resolve_local(hidden_name))
            }
        );
      } else {
        std::size_t hidden_idx =
            make_constant(runtime::Value{std::string{hidden_name}});
        emit(bytecode::OpGetGlobal{hidden_idx});
      }

      emit(
          bytecode::OpConstant{make_constant(
              runtime::Value{runtime::Primitive{static_cast<std::int64_t>(i)}}
          )}
      );
      emit(bytecode::OpGetIndex{});

      std::string name{names[i].get_name()};
      int local_arg = resolve_local(name);
      if (local_arg != -1) {
        emit(bytecode::OpSetLocal{static_cast<std::size_t>(local_arg)});
      } else {
        int upvalue_arg =
            resolve_upvalue(name, static_cast<int>(contexts.size()) - 1);
        if (upvalue_arg != -1) {
          emit(bytecode::OpSetUpvalue{static_cast<std::size_t>(upvalue_arg)});
        } else {
          std::size_t name_index =
              make_constant(runtime::Value{std::string{name}});
          emit(bytecode::OpSetGlobal{name_index});
        }
      }
    }
  }
}

void Compiler::compile_named_function(const ast::NamedFunction &func) {
  std::string name{func.get_name().get_name()};
  bool is_local = scope_depth > 0;

  if (is_local) {
    emit(bytecode::OpConstant{make_constant(runtime::Value{runtime::Nil{}})});
    locals.push_back({.name = name, .depth = scope_depth});
  }

  chunks.emplace_back();
  std::size_t new_chunk_id = chunks.size() - 1;
  push_context(new_chunk_id);

  const auto &args = func.get_body().get_parameters();
  for (const auto &arg : args) {
    locals.push_back(
        {.name = std::string{arg.get_name()}, .depth = scope_depth}
    );
  }

  compile_block(func.get_body().get_block());
  emit(bytecode::OpConstant{make_constant(runtime::Value{runtime::Nil{}})});
  emit(bytecode::OpReturn{});

  std::vector<bytecode::UpvalueInfo> upvalue_infos;
  upvalue_infos.reserve(current_upvalues.size());
  for (const auto &uv : current_upvalues) {
    upvalue_infos.emplace_back(uv.is_local, uv.index);
  }

  pop_context();

  std::size_t id = make_constant(
      runtime::Function{runtime::BytecodeFunctionId{
          .id = new_chunk_id,
          .name = name,
          .arity = args.size(),
          .upvalues = std::vector<runtime::Ref>(),
          .curried_args = std::vector<runtime::Ref>()
      }}
  );

  if (upvalue_infos.empty()) {
    emit(bytecode::OpConstant{id});
  } else {
    emit(
        bytecode::OpClosure{
            .function_index = id, .upvalues = std::move(upvalue_infos)
        }
    );
  }

  std::size_t name_index = make_constant(runtime::Value{std::string{name}});
  if (is_local) {
    emit(bytecode::OpMutateLocal{static_cast<std::size_t>(locals.size() - 1)});
  } else {
    emit(bytecode::OpDefineGlobal{name_index});
  }
}

void Compiler::compile_operator_assignment(
    const ast::OperatorAssignment &assign
) {
  assign.get_variable().visit(
      [&](const ast::Identifier &id) {
        std::string name{id.get_name()};
        int local_arg = resolve_local(name);
        int upvalue_arg = -1;
        std::size_t name_index = 0;

        if (local_arg == -1) {
          upvalue_arg =
              resolve_upvalue(name, static_cast<int>(contexts.size()) - 1);
        }

        if (assign.get_operator() != ast::AssignmentOperator::Assign) {
          if (local_arg != -1) {
            emit(bytecode::OpGetLocal{static_cast<std::size_t>(local_arg)});
          } else if (upvalue_arg != -1) {
            emit(bytecode::OpGetUpvalue{static_cast<std::size_t>(upvalue_arg)});
          } else {
            name_index = make_constant(runtime::Value{std::string{name}});
            emit(bytecode::OpGetGlobal{name_index});
          }
        }

        compile_expression(assign.get_expression());

        switch (assign.get_operator()) {
        case ast::AssignmentOperator::Assign:
          break;
        case ast::AssignmentOperator::Plus:
          emit(bytecode::OpAdd{});
          break;
        case ast::AssignmentOperator::Minus:
          emit(bytecode::OpSubtract{});
          break;
        case ast::AssignmentOperator::Multiply:
          emit(bytecode::OpMultiply{});
          break;
        case ast::AssignmentOperator::Divide:
          emit(bytecode::OpDivide{});
          break;
        case ast::AssignmentOperator::Modulo:
          emit(bytecode::OpModulo{});
          break;
        case ast::AssignmentOperator::Power:
          break; // TODO
        }

        if (local_arg != -1) {
          emit(bytecode::OpSetLocal{static_cast<std::size_t>(local_arg)});
        } else if (upvalue_arg != -1) {
          emit(bytecode::OpSetUpvalue{static_cast<std::size_t>(upvalue_arg)});
        } else {
          if (assign.get_operator() == ast::AssignmentOperator::Assign) {
            name_index = make_constant(runtime::Value{std::string{name}});
          }
          emit(bytecode::OpSetGlobal{name_index});
        }
      },
      [&](const ast::IndexExpression &idx) {
        if (assign.get_operator() != ast::AssignmentOperator::Assign) {
          compile_variable(idx.get_base());
          compile_expression(idx.get_index());
          compile_variable(idx.get_base());
          compile_expression(idx.get_index());
          emit(bytecode::OpGetIndex{});

          compile_expression(assign.get_expression());

          switch (assign.get_operator()) {
          case ast::AssignmentOperator::Plus:
            emit(bytecode::OpAdd{});
            break;
          case ast::AssignmentOperator::Minus:
            emit(bytecode::OpSubtract{});
            break;
          case ast::AssignmentOperator::Multiply:
            emit(bytecode::OpMultiply{});
            break;
          case ast::AssignmentOperator::Divide:
            emit(bytecode::OpDivide{});
            break;
          case ast::AssignmentOperator::Modulo:
            emit(bytecode::OpModulo{});
            break;
          case ast::AssignmentOperator::Power:
            break;
          default:
            std::unreachable();
          }

          emit(bytecode::OpSetIndex{});
        } else {
          compile_variable(idx.get_base());
          compile_expression(idx.get_index());
          compile_expression(assign.get_expression());
          emit(bytecode::OpSetIndex{});
        }
      }
  );
}

void Compiler::compile_range_for_loop(const ast::RangeForLoop &loop) {
  break_jumps_stack.emplace_back();
  continue_jumps_stack.emplace_back();
  begin_scope();

  compile_expression(loop.get_start());
  locals.push_back({.name = "_range_current", .depth = scope_depth});

  compile_expression(loop.get_end());
  locals.push_back({.name = "_range_end", .depth = scope_depth});

  if (loop.get_step()) {
    compile_expression(*loop.get_step());
  } else {
    emit(
        bytecode::OpConstant{
            make_constant(runtime::Value{runtime::Primitive{1L}})
        }
    );
  }
  locals.push_back({.name = "_range_step", .depth = scope_depth});

  std::size_t loop_start = current_instruction_offset();

  std::size_t current_idx =
      static_cast<std::size_t>(resolve_local("_range_current"));
  std::size_t end_idx = static_cast<std::size_t>(resolve_local("_range_end"));
  std::size_t step_idx = static_cast<std::size_t>(resolve_local("_range_step"));

  emit(bytecode::OpGetLocal{current_idx});
  emit(bytecode::OpGetLocal{end_idx});

  if (loop.get_range_type() == ast::RangeOperator::Inclusive) {
    emit(bytecode::OpLessEqual{});
  } else {
    emit(bytecode::OpLess{});
  }

  std::size_t exit_jump = emit_jump(bytecode::OpJumpIfFalse{});
  emit(bytecode::OpPop{});

  loop_body_locals_snapshot.push_back(locals.size());

  begin_scope();
  emit(bytecode::OpGetLocal{current_idx});
  locals.push_back(
      {.name = std::string{loop.get_variable().get_name()},
       .depth = scope_depth}
  );

  compile_block(loop.get_body());

  end_scope();
  loop_body_locals_snapshot.pop_back();

  // continue target: after end_scope(), before the increment block.
  std::size_t continue_target = current_instruction_offset();

  emit(bytecode::OpGetLocal{current_idx});
  emit(bytecode::OpGetLocal{step_idx});
  emit(bytecode::OpAdd{});
  emit(bytecode::OpSetLocal{current_idx});

  emit_loop(loop_start);

  patch_jump_here(exit_jump);
  emit(bytecode::OpPop{});

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
  std::size_t loop_start = current_chunk().code.size();
  // `continue` in a while loop jumps back to the condition check.

  compile_expression(loop.get_condition());

  std::size_t exit_jump = emit_jump(bytecode::OpJumpIfFalse{});

  emit(bytecode::OpPop{});

  // Snapshot locals before the body scope so continue can compute how many
  // body-scope locals to pop.
  loop_body_locals_snapshot.push_back(locals.size());
  compile_block(loop.get_body());
  loop_body_locals_snapshot.pop_back();

  emit_loop(loop_start);

  patch_jump_here(exit_jump);

  emit(bytecode::OpPop{});

  for (auto jump : break_jumps_stack.back()) {
    patch_jump_here(jump);
  }
  break_jumps_stack.pop_back();

  for (auto jump : continue_jumps_stack.back()) {
    current_chunk().code[jump] = bytecode::OpJump{loop_start};
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
    emit(bytecode::OpConstant{make_constant(runtime::Value{runtime::Nil{}})});
  }
  emit(bytecode::OpReturn{});
}

void Compiler::compile_break_statement(const ast::BreakStatement & /* brk */) {
  if (break_jumps_stack.empty()) {
    throw std::runtime_error("Break statement outside of loop");
  }
  break_jumps_stack.back().push_back(emit_jump(bytecode::OpJump{}));
}

void Compiler::
    compile_continue_statement(const ast::ContinueStatement & /* cont */) {
  if (continue_jumps_stack.empty()) {
    throw std::runtime_error("Continue statement outside of loop");
  }
  // Emit pops for locals introduced in the current loop body scope,
  // without mutating `locals` (the loop continues on the next iteration).
  std::size_t body_locals = locals.size() - loop_body_locals_snapshot.back();
  for (std::size_t i = 0; i < body_locals; ++i) {
    emit(bytecode::OpPop{});
  }
  continue_jumps_stack.back().push_back(emit_jump(bytecode::OpJump{}));
}

} // namespace l3::compiler
