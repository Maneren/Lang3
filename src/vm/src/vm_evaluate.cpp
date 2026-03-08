module l3.vm;

import utils;
import l3.ast;
import :formatting;
import :variable;

namespace l3::vm {

Ref VM::evaluate(const ast::UnaryExpression &unary) {
  debug_print("Evaluating unary expression {}", unary.get_op());
  const auto argument = evaluate(unary.get_expression());
  try {
    switch (unary.get_op()) {
    case ast::UnaryOperator::Minus:
      return store_value(argument->negative());
    case ast::UnaryOperator::Plus:
      return argument;
    case ast::UnaryOperator::Not:
      return store_value(argument->not_op());
    }
    throw std::runtime_error("unreachable");
  } catch (RuntimeError &error) {
    error.set_location(unary.get_location());
    throw;
  }
}

Ref VM::evaluate(const ast::BinaryExpression &binary) {
  debug_print("Evaluating binary expression {}", binary.get_op());
  const auto &left = *evaluate(binary.get_lhs());
  const auto &right = *evaluate(binary.get_rhs());

  debug_print("  Left: {}", left);
  debug_print("  Right: {}", right);

  try {
    switch (binary.get_op()) {
    case ast::BinaryOperator::Plus:
      return store_value(left.add(right));
    case ast::BinaryOperator::Minus:
      return store_value(left.sub(right));
    case ast::BinaryOperator::Multiply:
      return store_value(left.mul(right));
    case ast::BinaryOperator::Divide:
      return store_value(left.div(right));
    case ast::BinaryOperator::Modulo:
      return store_value(left.mod(right));
    default:
      throw std::runtime_error(
          std::format("not implemented: {}", binary.get_op())
      );
    }
  } catch (RuntimeError &error) {
    error.set_location(binary.get_location());
    throw;
  }
}

Ref VM::evaluate(const ast::LogicalExpression &logical) {
  const auto op = logical.get_op();
  debug_print("Evaluating logical expression {}", op);

  auto lhs = [this, &logical] {
    const auto left = evaluate(logical.get_lhs());
    debug_print("  Left: {}", *left);
    return left;
  };
  auto rhs = [this, &logical] {
    const auto right = evaluate(logical.get_rhs());
    debug_print("  Right: {}", *right);
    return right;
  };

  switch (op) {
  case ast::LogicalOperator::And: {
    const auto left = lhs();
    if (!left->is_truthy()) {
      debug_print("  Left is falsy, short-circuiting {}", op);
      return left;
    }
    return rhs();
  }
  case ast::LogicalOperator::Or: {
    const auto left = lhs();
    if (left->is_truthy()) {
      debug_print("  Left is truthy, short-circuiting {}", op);
      return left;
    }
    return rhs();
  }
  default:
    throw std::runtime_error(
        std::format("not implemented: {}", logical.get_op())
    );
  }
}

Ref VM::evaluate(const ast::Comparison &chained) {
  debug_print("Evaluating comparison");
  const auto &operands = chained.get_comparisons();

  auto lhs = evaluate(chained.get_start());

  debug_print("  Start: {}", *lhs);
  debug_print("  Comparisons:");

  for (const auto &[op, right] : operands) {
    const auto rhs = evaluate(right);

    const auto comparison = lhs->compare(*rhs);
    debug_print("  Comparing {} {} {}", *lhs, op, *rhs);
    debug_print("  Result: {}", comparison);

    switch (op) {
      using namespace ast;

    case ComparisonOperator::Equal:
      if (comparison != 0) {
        return _false();
      }
      break;
    case ComparisonOperator::NotEqual:
      if (comparison == 0) {
        return _false();
      }
      break;
    case ComparisonOperator::Less:
      if (comparison >= 0) {
        return _false();
      }
      break;
    case ComparisonOperator::LessEqual:
      if (comparison > 0) {
        return _false();
      }
      break;
    case ComparisonOperator::Greater:
      if (comparison <= 0) {
        return _false();
      }
      break;
    case ComparisonOperator::GreaterEqual:
      if (comparison < 0) {
        return _false();
      }
      break;
    default:
      throw std::runtime_error(std::format("not implemented: {}", op));
    }

    lhs = rhs;
  }

  return _true();
}

Ref VM::evaluate(const ast::Identifier &identifier) {
  try {
    return read_variable(identifier);
  } catch (RuntimeError &error) {
    error.set_location(identifier.get_location());
    throw;
  }
}

Ref VM::evaluate(const ast::Variable &variable) {
  return variable.visit([this](const auto &inner) { return evaluate(inner); });
}

Ref VM::evaluate(const ast::Literal &literal) {
  debug_print("Evaluating literal");
  Value value = literal.visit(
      [](const ast::Nil & /*unused*/) -> Value { return {Nil{}}; },
      [this](const ast::Array &array) -> Value {
        Value::vector_type values;
        values.reserve(array.get_elements().size());
        for (const auto &element : array.get_elements()) {
          values.push_back(evaluate(element));
        }
        return {std::move(values)};
      },
      [](const ast::String &string) -> Value {
        return {std::string{string.get_value()}};
      },
      [](const auto &literal_value) -> Value {
        return Primitive{literal_value.get_value()};
      }
  );
  debug_print("Literal: {}", value);

  return store_value(std::move(value));
}

Ref VM::evaluate(const ast::Expression &expression) {
  return expression.visit([this](const auto &expression) {
    auto result = evaluate(expression);
    debug_print("Expression result: {}", result.get());
    return result;
  });
}

std::vector<Ref> VM::evaluate(const ast::ExpressionList &expressions) {
  std::vector<Ref> result;
  result.reserve(expressions.size());
  for (const auto &expression : expressions) {
    result.push_back(evaluate(expression));
  }
  return result;
}

Ref VM::evaluate(const BuiltinFunction &function, L3Args arguments) {
  return function.invoke(*this, arguments);
}

Ref VM::evaluate(const L3Function &function, L3Args arguments) {
  const auto &body = function.get_body().get();
  const auto parameters = body.get_parameters();
  const auto &curried = function.get_curried();

  Scope argument_scope;
  if (curried.has_value()) {
    argument_scope = curried->get().clone(*this);
  } else {
    argument_scope = {};
  }

  auto needed_arguments = parameters.size() - argument_scope.size();

  if (arguments.size() > needed_arguments) {
    throw RuntimeError{
        "Function {} expected at most {} arguments, got {}",
        function,
        needed_arguments,
        arguments.size()
    };
  }

  for (auto [parameter, argument] :
       std::views::zip(parameters.subspan(argument_scope.size()), arguments)) {
    argument_scope.declare_variable(parameter, argument, Mutability::Mutable);
  }

  if (arguments.size() < needed_arguments) {
    debug_print("Returning curried function", function);
    return store_value(Function{function.curry(std::move(argument_scope))});
  }

  const auto &captures = function.get_captures();
  for (const auto &capture : captures->get_scopes()) {
    debug_print("captured: {}", capture->get_variables());
  }
  debug_print("Evaluating function body");

  ExecutionState function_state{captures};
  function_state.in_function = true;
  const auto overlay =
      ExecutionState::Overlay{*this, std::move(function_state)};
  const auto scope_guard =
      state().scope_stack->with_frame(std::move(argument_scope));

  execute(body.get_block());

  if (state().flow_control == FlowControl::Return) {
    auto value = *state().return_value;
    state().return_value = std::nullopt;
    state().flow_control = FlowControl::Normal;
    debug_print("Returning from function: {}", value);
    return stack.push_value(value);
  }

  return nil();
}

Ref VM::evaluate(const Function &function, L3Args arguments) {
  return function.visit([&, this](const auto &function) {
    return evaluate(function, arguments);
  });
}

Ref VM::evaluate(const ast::FunctionCall &function_call) {
  const auto &function_name = function_call.get_name();
  const auto &argument_expressions = function_call.get_arguments();

  debug_print("Calling function {}", function_name);

  const auto evaluated_function = evaluate(function_name);

  const Function &function = evaluated_function->visit(
      [](const Value::function_type &function) -> const Function & {
        return *function;
      },
      [](const auto &value) -> const Function & {
        throw std::runtime_error(std::format("{} is not a function", value));
      }
  );

  auto arguments = evaluate(argument_expressions);

  debug_print("Arguments:");
  for (const auto &argument : arguments) {
    debug_print("  {}", argument);
  }

  const auto call_stack_guard = CallStackGuard{
      this->call_stack,
      {.function_name = function_name, .location = function_call.get_location()}
  };

  Ref result = nil();
  try {
    result = evaluate(function, arguments);
  } catch (RuntimeError &error) {
    error.set_stack_trace(call_stack);
    throw;
  }

  debug_print("Result: {}", result);
  return result;
}

Ref VM::evaluate(const ast::AnonymousFunction &anonymous) {
  return store_value(
      Function{L3Function{state().scope_stack->clone(*this), anonymous}}
  );
}

Ref VM::evaluate(const ast::IndexExpression &index_expression) {
  auto base = evaluate(index_expression.get_base());
  auto index = evaluate(index_expression.get_index());
  try {
    return store_new_value(base->index(*index));
  } catch (RuntimeError &error) {
    error.set_location(index_expression.get_location());
    throw;
  }
}

Ref &VM::evaluate_mut(const ast::Variable &variable) {
  return variable.visit([this](const auto &variable) -> Ref & {
    return evaluate_mut(variable);
  });
}

Ref &VM::evaluate_mut(const ast::Identifier &identifier) {
  try {
    debug_print("Reading variable {}", identifier.get_name());
    return read_write_variable(identifier);
  } catch (RuntimeError &error) {
    error.set_location(identifier.get_location());
    throw;
  }
}

Ref &VM::evaluate_mut(const ast::IndexExpression &index_expression) {
  auto base = evaluate_mut(index_expression.get_base());
  auto index = evaluate(index_expression.get_index());
  try {
    return base->index_mut(*index);
  } catch (RuntimeError &error) {
    error.set_location(index_expression.get_location());
    throw;
  }
}

bool VM::evaluate_if_branch(const ast::IfBase &if_base) {
  debug_print("Evaluating if branch");
  const auto condition_value = evaluate(if_base.get_condition());
  const auto &condition = condition_value.get();
  if (condition.is_truthy()) {
    debug_print("Condition is truthy {}", condition_value);
    execute(if_base.get_block());
    return true;
  }

  debug_print("Condition is falsy {}", condition_value);
  return false;
}

Ref VM::evaluate(const ast::IfExpression &if_expr) {
  execute(static_cast<const ast::IfElseBase &>(if_expr));

  if (state().flow_control == FlowControl::Return) {
    auto value = *state().return_value;
    state().return_value = std::nullopt;
    state().flow_control = FlowControl::Normal;
    debug_print("Returning from if expression: {}", value);
    return value;
  }

  execute(if_expr.get_else_block());

  if (state().flow_control == FlowControl::Return) {
    auto value = *state().return_value;
    state().return_value = std::nullopt;
    state().flow_control = FlowControl::Normal;
    debug_print("Returning from if expression: {}", value);
    return value;
  }

  throw RuntimeError(
      "if expression did not return a value", if_expr.get_location()
  );
}

} // namespace l3::vm
