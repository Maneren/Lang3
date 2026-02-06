module;

#include <ranges>
#include <vector>

module l3.vm;

import utils;
import l3.ast;
import :formatting;
import :variable;

namespace l3::vm {

RefValue VM::evaluate(const ast::UnaryExpression &unary) {
  debug_print("Evaluating unary expression {}", unary.get_op());
  const auto argument = evaluate(unary.get_expression());
  switch (unary.get_op()) {
  case ast::UnaryOperator::Minus:
    return store_value(argument->negative());
  case ast::UnaryOperator::Plus:
    return argument;
  case ast::UnaryOperator::Not:
    return store_value(argument->not_op());
  }
  throw std::runtime_error("unreachable");
}

RefValue VM::evaluate(const ast::BinaryExpression &binary) {
  debug_print("Evaluating binary expression {}", binary.get_op());
  const auto &left = *evaluate(binary.get_lhs());
  const auto &right = *evaluate(binary.get_rhs());

  debug_print("  Left: {}", left);
  debug_print("  Right: {}", right);

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
}

RefValue VM::evaluate(const ast::LogicalExpression &logical) {
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

RefValue VM::evaluate(const ast::Comparison &chained) {
  debug_print("Evaluating comparison");
  const auto &operands = chained.get_comparisons();

  auto lhs = evaluate(chained.get_start());

  debug_print("  Start: {}", *lhs);
  debug_print("  Comparisons:");

  for (const auto &[op, right] : operands) {
    const auto rhs = evaluate(*right);

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

RefValue VM::evaluate(const ast::Identifier &identifier) {
  return read_variable(identifier);
}

RefValue VM::evaluate(const ast::Variable &variable) {
  return variable.visit([this](const auto &inner) { return evaluate(inner); });
}

RefValue VM::evaluate(const ast::Literal &literal) {
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

RefValue VM::evaluate(const ast::Expression &expression) {
  debug_print("Evaluating expression");
  return expression.visit([this](const auto &expression) {
    auto result = evaluate(expression);
    debug_print("Expression result: {}", result.get());
    return result;
  });
}

std::vector<RefValue> VM::evaluate(const ast::ExpressionList &expressions) {
  std::vector<RefValue> result;
  result.reserve(expressions.size());
  for (const auto &expression : expressions) {
    result.push_back(evaluate(expression));
  }
  return result;
}

RefValue VM::evaluate(const BuiltinFunction &function, L3Args arguments) {
  return function.invoke(*this, arguments);
}

RefValue VM::evaluate(const L3Function &function, L3Args arguments) {
  const auto &body = function.get_body().get();
  const auto parameters = body.get_parameters();
  const auto &curried = function.get_curried();

  Scope argument_scope;
  if (curried.get() != nullptr) {
    argument_scope = curried->clone(*this);
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

  const auto overlay = ScopeStackOverlay{*this, captures};
  const auto scope_guard = scopes->with_frame(std::move(argument_scope));

  execute(body.get_block());

  if (flow_control == FlowControl::Return) {
    auto value = *return_value;
    return_value = std::nullopt;
    flow_control = FlowControl::Normal;
    debug_print("Returning from function: {}", value);
    return stack.push_value(value);
  }

  return nil();
}

RefValue VM::evaluate(const Function &function, L3Args arguments) {
  return function.visit([&, this](const auto &function) {
    return evaluate(function, arguments);
  });
}

RefValue VM::evaluate(const ast::FunctionCall &function_call) {
  const auto &function_name = function_call.get_name();
  const auto &argument_expressions = function_call.get_arguments();

  debug_print("Calling function {}", function_name);

  const auto evaluated_function = evaluate(function_name);

  const Function &function = evaluated_function->visit(
      [](const Function &function) -> const Function & { return function; },
      [](const auto &value) -> const Function & {
        throw std::runtime_error(std::format("{} is not a function", value));
      }
  );

  auto arguments = evaluate(argument_expressions);

  debug_print("Arguments:");
  for (const auto &argument : arguments) {
    debug_print("  {}", argument);
  }

  const auto result = evaluate(function, arguments);

  switch (flow_control) {
  case FlowControl::Break:
  case FlowControl::Continue:
    throw RuntimeError("Unexpected {} outside a loop", flow_control);
  default:
    break;
  }

  debug_print("Result: {}", result);
  return result;
}

RefValue VM::evaluate(const ast::AnonymousFunction &anonymous) {
  return store_value(Function{L3Function{scopes, anonymous}});
}

RefValue VM::evaluate(const ast::IndexExpression &index_expression) {
  auto base = evaluate(index_expression.get_base());
  auto index = evaluate(index_expression.get_index());
  return store_new_value(base->index(*index));
}

RefValue &VM::evaluate_mut(const ast::Variable &variable) {
  return variable.visit([this](const auto &variable) -> RefValue & {
    return evaluate_mut(variable);
  });
}

RefValue &VM::evaluate_mut(const ast::Identifier &identifier) {
  return read_write_variable(identifier);
}

RefValue &VM::evaluate_mut(const ast::IndexExpression &index_expression) {
  auto base = evaluate_mut(index_expression.get_base());
  auto index = evaluate(index_expression.get_index());
  return base->index_mut(*index);
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

RefValue VM::evaluate(const ast::IfExpression &if_expr) {
  execute(static_cast<const ast::IfElseBase &>(if_expr));

  if (flow_control == FlowControl::Return) {
    auto value = *return_value;
    return_value = std::nullopt;
    flow_control = FlowControl::Normal;
    debug_print("Returning from if expression: {}", value);
    return value;
  }

  execute(if_expr.get_else_block());

  if (flow_control == FlowControl::Return) {
    auto value = *return_value;
    return_value = std::nullopt;
    flow_control = FlowControl::Normal;
    debug_print("Returning from if expression: {}", value);
    return value;
  }

  throw RuntimeError("if expression did not return a value");
}

} // namespace l3::vm
