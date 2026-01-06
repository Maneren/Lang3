#include "vm/vm.hpp"
#include <algorithm>
#include <iostream>
#include <print>
#include <ranges>

namespace l3::vm {

CowValue VM::evaluate(const ast::BinaryExpression &binary) {
  std::println(std::cerr, "Evaluating binary expression {}", binary.get_op());
  const auto left = evaluate(binary.get_lhs());
  const auto right = evaluate(binary.get_rhs());

  std::println(std::cerr, "Left: {}", left);
  std::println(std::cerr, "Right: {}", right);

  const auto &left_ref = left.as_ref();
  const auto &right_ref = right.as_ref();

  switch (binary.get_op()) {
  case ast::BinaryOperator::Plus: {
    return CowValue{left_ref.add(right_ref)};
  }
  case ast::BinaryOperator::Minus: {
    return CowValue{left_ref.sub(right_ref)};
  }
  case ast::BinaryOperator::Multiply: {
    return CowValue{left_ref.mul(right_ref)};
  }
  case ast::BinaryOperator::Divide: {
    return CowValue{left_ref.div(right_ref)};
  }
  case ast::BinaryOperator::Modulo: {
    return CowValue{left_ref.mod(right_ref)};
  }
  case ast::BinaryOperator::And: {
    return CowValue{left_ref.and_op(right_ref)};
  }
  case ast::BinaryOperator::Or: {
    return CowValue{left_ref.or_op(right_ref)};
  }
  default:
    throw std::runtime_error(
        std::format("not implemented: {}", binary.get_op())
    );
  }
}

CowValue VM::evaluate(const ast::Identifier &identifier) {
  if (auto value = get_variable(identifier)) {
    return CowValue{*value};
  }
  throw UndefinedVariableError(identifier);
}

CowValue VM::evaluate(const ast::Variable &variable) {
  return evaluate(variable.get_identifier());
}

CowValue VM::evaluate(const ast::Literal &literal) {
  std::println(std::cerr, "Evaluating literal");
  Value primitive = match::match(
      literal.get(),
      [](const ast::Nil & /*unused*/) -> Value { return {Nil{}}; },
      [](const auto &literal_value) -> Value {
        return Primitive{literal_value.get()};
      }
  );
  std::println(std::cerr, "Primitive: {}", primitive);

  return CowValue{std::move(primitive)};
}
CowValue VM::evaluate(const ast::Expression &expression) {
  std::println(std::cerr, "Evaluating expression");
  return expression.visit([this](const auto &expr) {
    auto result = evaluate(expr);
    std::println(std::cerr, "Expression result: {}", result.as_ref());
    return result;
  });
}
void VM::execute(const ast::Assignment &assignment) {
  const auto &variable = assignment.get_variable();
  std::println(
      std::cerr, "Executing assignment to {}", variable.get_identifier().name()
  );
  auto value = get_variable(variable.get_identifier());
  if (!value) {
    throw UndefinedVariableError(variable);
  }
  const auto &expression = assignment.get_expression();
  auto &lhs = value->get();
  const auto rhs = evaluate(expression);
  const auto &rhs_ref = rhs.as_ref();
  switch (assignment.get_operator()) {
  case ast::AssignmentOperator::Assign:
    lhs = rhs_ref;
    break;
  case ast::AssignmentOperator::Plus:
    lhs = lhs.add(rhs_ref);
    break;
  case ast::AssignmentOperator::Minus:
    lhs = lhs.sub(rhs_ref);
    break;
  case ast::AssignmentOperator::Multiply:
    lhs = lhs.mul(rhs_ref);
    break;
  case ast::AssignmentOperator::Divide:
    lhs = lhs.div(rhs_ref);
    break;
  case ast::AssignmentOperator::Modulo:
    lhs = lhs.mod(rhs_ref);
    break;
  default:
    throw std::runtime_error(
        std::format("not implemented: {}", assignment.get_operator())
    );
  }
  std::println(std::cerr, "Assigned: {}", value->get());
}
void VM::execute(const ast::Declaration &declaration) {
  const auto &variable = declaration.get_variable();
  const auto &expression = declaration.get_expression();
  std::println(std::cerr, "Executing declaration of {}", variable.name());
  auto &value = current_scope().declare_variable(variable);
  value = evaluate(expression).into_value();
  std::println(std::cerr, "Declared {} = {}", variable.name(), value);
}
void VM::execute(const ast::FunctionCall &function_call) {
  const auto &value = evaluate(function_call);
  if (!value->is_nil()) {
    throw RuntimeError("Top-value function call returned non-nil value");
  }
}
void VM::execute(const ast::IfStatement &if_statement) {
  std::println(std::cerr, "Evaluating if statement");
  if (evaluate_if_branch(if_statement.get_base_if()) ||
      std::ranges::any_of(
          if_statement.get_elseif(),
          std::bind_front(&VM::evaluate_if_branch, this)
      )) {
    return;
  }

  std::println(std::cerr, "Executing else block");
  if (auto else_block = if_statement.get_else_block(); else_block) {
    execute(else_block->get());
  }
}
CowValue VM::evaluate(const ast::FunctionCall &function_call) {
  const auto &function = function_call.get_name();
  const auto &arguments = function_call.get_arguments();
  auto evaluated_function = evaluate(function);

  Value::function_type function_ptr = evaluated_function->visit(
      [](const Value::function_type &function) { return function; },
      [](const auto &value) -> Value::function_type {
        throw std::runtime_error(std::format("{} is not a function", value));
      }
  );

  const auto evaluated_arguments =
      std::views::transform(
          arguments, [this](const auto &argument) { return evaluate(argument); }
      ) |
      std::ranges::to<std::vector>();

  std::println(std::cerr, "Arguments:");
  for (const auto &argument : evaluated_arguments) {
    std::println(std::cerr, "  {}", argument);
  }

  return function_ptr->operator()(evaluated_arguments);
};
void VM::execute(const ast::Statement &statement) {
  std::println(std::cerr, "Executing statement");
  statement.visit([this](const auto &stmt) { execute(stmt); });
}
void VM::execute(const ast::Program &program) {
  CPPTRACE_TRY {
    try {
      for (const auto &statement : program.get_statements()) {
        execute(statement);
      }
    } catch (const RuntimeError &error) {
      std::println(std::cerr, "{}", error.what());
    }
  }
  CPPTRACE_CATCH(const std::exception &e) {
    std::cerr << "Exception: " << e.what() << '\n';
    cpptrace::from_current_exception().print();
  }
}

[[nodiscard]] std::optional<std::reference_wrapper<Value>>
VM::get_variable(const ast::Identifier &id) const {
  for (const auto &it : std::views::reverse(scopes)) {
    if (auto value = it.get_variable(id)) {
      return value;
    }
  }
  return std::nullopt;
}

bool VM::evaluate_if_branch(const ast::IfBase &if_base) {
  std::println(std::cerr, "Evaluating if branch");
  const auto condition_value = evaluate(if_base.get_condition());
  const auto &condition = condition_value.as_ref();
  if (condition.as_bool()) {
    std::println(std::cerr, "Condition is truthy {}", condition_value);
    execute(if_base.get_block());
    return true;
  }

  std::println(std::cerr, "Condition is falsy {}", condition_value);
  return false;
};

} // namespace l3::vm
