module l3.vm;

import utils;
import l3.ast;
import :formatting;
import :variable;
import :loop_guard;

namespace l3::vm {

void VM::execute(const ast::OperatorAssignment &assignment) {
  const auto &variable = assignment.get_variable();
  debug_print("Executing operator assignment");
  auto &lhs = evaluate_mut(variable);
  const auto &expression = assignment.get_expression();
  const auto rhs = evaluate(expression);

  debug_print("  LHS: {}", lhs);
  debug_print("  RHS: {}", rhs);

  try {
    switch (assignment.get_operator()) {
    case ast::AssignmentOperator::Assign:
      lhs = rhs;
      break;
    case ast::AssignmentOperator::Plus:
      lhs->add_assign(*rhs);
      break;
    case ast::AssignmentOperator::Minus:
      lhs = store_value(lhs->sub(*rhs));
      break;
    case ast::AssignmentOperator::Multiply:
      lhs->mul_assign(*rhs);
      break;
    case ast::AssignmentOperator::Divide:
      lhs = store_value(lhs->div(*rhs));
      break;
    case ast::AssignmentOperator::Modulo:
      lhs = store_value(lhs->mod(*rhs));
      break;
    default:
      throw std::runtime_error(
          std::format("not implemented: {}", assignment.get_operator())
      );
    }
    debug_print("Assigned: {}", lhs);
  } catch (RuntimeError &error) {
    error.set_location(assignment.get_location());
    throw;
  }
}

bool VM::execute(const ast::ElseIfList &elseif_list) {
  return std::ranges::any_of(
      elseif_list.get_elseifs(), std::bind_front(&VM::evaluate_if_branch, this)
  );
}

void VM::execute(const ast::IfStatement &if_statement) {
  debug_print("Evaluating if statement");
  if (evaluate_if_branch(if_statement.get_base_if()) ||
      execute(if_statement.get_elseif())) {
    return;
  }

  debug_print("Executing else block");
  if (auto else_block = if_statement.get_else_block(); else_block) {
    execute(else_block->get());
  }
}

void VM::execute(const ast::IfElseBase &if_else_base) {
  if (evaluate_if_branch(if_else_base.get_base_if()) ||
      execute(if_else_base.get_elseif())) {
    return;
  }
}

void VM::execute(const ast::FunctionCall &function_call) {
  auto _ = evaluate(function_call);
}

void VM::execute(const ast::Statement &statement) {
  debug_print("Executing statement");
  statement.visit([this](const auto &stmt) { execute(stmt); });
  run_gc();
}

void VM::execute(const ast::Program &program) {
  try {
    execute(static_cast<const ast::Block &>(program));
  } catch (const RuntimeError &error) {
    std::println(std::cerr, "{}", error.format_error());
  }
}

void VM::execute(const ast::Block &block) {
  debug_print("Evaluating block");
  const auto frame_guard = stack.with_frame();
  const auto scope_guard = state().scope_stack->with_frame();
  for (const auto &statement : block.get_statements()) {
    execute(statement);

    if (state().flow_control != FlowControl::Normal) {
      return;
    }
  }

  const auto &last_statement = block.get_last_statement();

  if (last_statement) {
    execute(*last_statement);
  }
}

void VM::execute(const ast::NamedFunction &named_function) {
  debug_print("Declaring named function");
  const auto &name = named_function.get_name();
  declare_variable(
      name,
      Mutability::Immutable,
      store_value(
          Function{L3Function{state().scope_stack->clone(*this), named_function}}
      )
  );

  debug_print("Declared function {}", name.get_name());
}

void VM::execute(const ast::LastStatement &last_statement) {
  debug_print("Evaluating last statement");
  last_statement.visit(
      [this](const ast::ReturnStatement &return_statement) {
        if (!state().in_function) {
          throw RuntimeError(
              return_statement.get_location(),
              "return statement outside of function"
          );
        }

        state().return_value = return_statement.get_expression()
                                   .transform([this](const auto &expression) {
                                     return evaluate(expression);
                                   })
                                   .value_or(VM::nil());

        state().flow_control = FlowControl::Return;
        debug_print("Returning {}", *state().return_value);
      },
      [this](const ast::BreakStatement &break_stmt) {
        if (!state().in_loop) {
          throw RuntimeError(
              break_stmt.get_location(), "break statement outside of loop"
          );
        }
        state().flow_control = FlowControl::Break;
      },
      [this](const ast::ContinueStatement &continue_stmt) {
        if (!state().in_loop) {
          throw RuntimeError(
              continue_stmt.get_location(), "continue statement outside of loop"
          );
        }
        state().flow_control = FlowControl::Continue;
      }
  );
}

void VM::execute(const ast::Declaration &declaration) {
  debug_print("Executing declaration");
  const auto &names = declaration.get_names();

  const auto mutability = declaration.get_mutability();

  const auto &expr_opt = declaration.get_expression();

  if (!expr_opt) {
    // No initializer, assign nil to all variables
    for (const auto &name : names) {
      declare_variable(name, mutability);
    }
    return;
  }

  auto value = evaluate(*expr_opt);

  if (names.size() == 1) {
    const auto &name = names.front();
    declare_variable(name, mutability, value);
    return;
  }

  const auto value_vector_opt = value->as_vector();
  if (!value_vector_opt) {
    throw ValueError(
        "Destructuring declaration only works with vectors",
        declaration.get_location()
    );
  }
  const auto &value_vector = value_vector_opt->get();

  if (value_vector.size() != names.size()) {
    throw ValueError(
        expr_opt->get_location(),
        "Destructuring declaration expected {} values but got {}",
        names.size(),
        value_vector.size()
    );
  }

  for (const auto [name, value] : std::views::zip(names, value_vector)) {
    declare_variable(name, mutability, value);
  }
}

void VM::execute(const ast::NameAssignment &assignment) {
  const auto &names = assignment.get_names();
  const auto value = evaluate(assignment.get_expression());
  const auto &location = assignment.get_location();

  debug_print(
      "Executing name assignment {} = {}",
      std::views::transform(names, &ast::Identifier::get_name) |
          std::ranges::to<std::vector>(),
      value
  );

  auto variables =
      std::views::transform(
          names,
          [&, this](const auto &id) { return std::ref(evaluate_mut(id)); }
      ) |
      std::ranges::to<std::vector>();

  if (variables.size() == 1) {
    variables.begin()->get() = value;
    return;
  }

  const auto value_vector_opt = value->as_vector();

  if (!value_vector_opt) {
    throw ValueError(
        "Destructuring assignment only works with vectors", location
    );
  }

  const auto &value_vector = value_vector_opt->get();

  if (value_vector.size() != variables.size()) {
    throw ValueError(
        location,
        "Destructuring assignment expected {} names but got {}",
        variables.size(),
        value_vector.size()
    );
  }

  for (const auto [variable, value] :
       std::views::zip(variables, value_vector)) {
    variable.get() = value;
  }
}

void VM::execute(const ast::While &while_loop) {
  const auto loop_guard = LoopGuard{state()};
  const auto &condition = while_loop.get_condition();
  const auto &body = while_loop.get_body();

  while (evaluate(condition)->is_truthy()) {
    execute(body);

    switch (state().flow_control) {
    case FlowControl::Normal:
      break;
    case FlowControl::Continue:
      state().flow_control = FlowControl::Normal;
      debug_print("Continue in a while loop");
      break;
    case FlowControl::Break:
      state().flow_control = FlowControl::Normal;
      debug_print("Break in a while loop");
      return;
    default:
      return;
    }
  }
}

void VM::execute(const ast::ForLoop &for_loop) {
  const auto loop_guard = LoopGuard{state()};
  const auto &variable = for_loop.get_variable();
  const auto &collection = for_loop.get_collection();
  const auto &body = for_loop.get_body();
  const auto mutability = for_loop.get_mutability();

  const auto scope_guard = state().scope_stack->with_frame();
  const auto frame_guard = stack.with_frame();
  auto &value = declare_variable(variable, mutability);

  const auto loop_body = [&](const auto &item) -> bool {
    *value = item;
    execute(body);

    switch (state().flow_control) {
    case FlowControl::Normal:
      return true;
    case FlowControl::Continue:
      state().flow_control = FlowControl::Normal;
      debug_print("Continue in a for loop");
      return true;
    case FlowControl::Break:
      state().flow_control = FlowControl::Normal;
      debug_print("Break in a for loop");
      return false;
    default:
      return false;
    }
  };

  const auto collection_value = evaluate(collection);
  if (const auto vector_opt = collection_value->as_vector()) {
    const auto &vector = vector_opt->get();
    for (const auto &item : vector) {
      if (!loop_body(item)) {
        return;
      }
    }
    return;
  }

  if (const auto string_opt = collection_value->as_string()) {
    const auto &string = string_opt->get();
    for (const char c : string) {
      if (!loop_body(store_value(std::string{c}))) {
        return;
      }
    }
    return;
  }

  throw TypeError(
      for_loop.get_location(),
      "cannot iterate over value of type '{}'",
      collection_value->type_name()
  );
}

void VM::execute(const ast::RangeForLoop &range_for_loop) {
  const auto loop_guard = LoopGuard{state()};
  const auto &variable = range_for_loop.get_variable();
  const auto &start_expr = range_for_loop.get_start();
  const auto &end_expr = range_for_loop.get_end();
  const auto &step_expr = range_for_loop.get_step();
  const auto &body = range_for_loop.get_body();
  const auto range_type = range_for_loop.get_range_type();
  const auto mutability = range_for_loop.get_mutability();

  const auto start_value =
      evaluate(start_expr)->as_primitive().and_then(&Primitive::as_integer);
  const auto end_value =
      evaluate(end_expr)->as_primitive().and_then(&Primitive::as_integer);

  if (!start_value || !end_value) {
    throw TypeError(
        "range bounds must be integers",
        !start_value ? start_expr.get_location() : end_expr.get_location()
    );
  }

  const std::int64_t start = *start_value;
  std::int64_t end = *end_value;
  std::int64_t step = 1;

  if (step_expr) {
    const auto step_value =
        evaluate(*step_expr)->as_primitive().and_then(&Primitive::as_integer);

    if (!step_value) {
      throw TypeError(
          "range step must be an integer", step_expr->get_location()
      );
    }

    step = *step_value;
    if (step == 0) {
      throw RuntimeError(
          "range step cannot be zero", step_expr->get_location()
      );
    }
  }

  if (range_type == ast::RangeOperator::Inclusive) {
    end += step;
  }

  auto full_range =
      step > 0 ? std::views::iota(start, end) : std::views::iota(end, start);

  auto range = full_range | std::views::stride(std::abs(step));

  const auto scope_guard = state().scope_stack->with_frame();
  const auto frame_guard = stack.with_frame();
  auto &value = declare_variable(variable, mutability);

  const auto loop_body = [&](std::int64_t i) -> bool {
    *value = store_value(Primitive{i});
    execute(body);

    switch (state().flow_control) {
    case FlowControl::Continue:
      state().flow_control = FlowControl::Normal;
      debug_print("Continue in a range for loop");
    case FlowControl::Normal:
      return false;
    case FlowControl::Break:
      state().flow_control = FlowControl::Normal;
      debug_print("Break in a range for loop");
    case FlowControl::Return:
      return true;
    }
  };

  std::ranges::any_of(range, loop_body);
}

} // namespace l3::vm
