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
        return {
            std::views::transform(
                array.get_elements(),
                [this](const auto &element) -> RefValue {
                  return evaluate(element);
                }
            ) |
            std::ranges::to<std::vector>()
        };
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

RefValue VM::evaluate(const ast::FunctionCall &function_call) {
  const auto &function = function_call.get_name();
  const auto &arguments = function_call.get_arguments();

  debug_print("Calling function {}", function);

  const auto evaluated_function = evaluate(function);

  const Value::function_type &function_ptr = evaluated_function->visit(
      [](const Value::function_type &function) -> Value::function_type {
        return function;
      },
      [](const auto &value) -> Value::function_type {
        throw std::runtime_error(std::format("{} is not a function", value));
      }
  );

  auto evaluated_arguments =
      arguments |
      std::views::transform([this](const auto &arg) { return evaluate(arg); }) |
      std::ranges::to<std::vector>();

  debug_print("Arguments:");
  for (const auto &argument : evaluated_arguments) {
    debug_print("  {}", argument);
  }

  auto result = function_ptr->operator()(*this, evaluated_arguments);

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
  return store_value(std::make_shared<Function>(L3Function{scopes, anonymous}));
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

RefValue VM::evaluate_function_body(
    const std::shared_ptr<ScopeStack> &captures,
    Scope &&arguments,
    const ast::FunctionBody &body
) {
  for (const auto &capture : captures->get_scopes()) {
    debug_print("captured: {}", capture->get_variables());
  }
  debug_print("arguments: {}", arguments.get_variables());

  const auto overlay = ScopeStackOverlay{*this, captures};
  const auto scope_guard = scopes->with_frame(std::move(arguments));

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
