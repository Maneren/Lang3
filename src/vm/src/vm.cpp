#include "vm/vm.hpp"
#include "vm/error.hpp"
#include "vm/format.hpp"
#include "vm/value.hpp"
#include <algorithm>
#include <ast/ast.hpp>
#include <cpptrace/from_current.hpp>
#include <cpptrace/from_current_macros.hpp>
#include <exception>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <stdexcept>
#include <utility>
#include <utils/match.h>
#include <vector>

namespace l3::vm {

CowValue VM::evaluate(const ast::BinaryExpression &binary) {
  debug_print("Evaluating binary expression {}", binary.get_op());
  const auto left = evaluate(binary.get_lhs());
  const auto right = evaluate(binary.get_rhs());

  debug_print("Left: {}", left);
  debug_print("Right: {}", right);

  const auto &left_ref = left.as_cref();
  const auto &right_ref = right.as_cref();

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
  case ast::BinaryOperator::Equal: {
    return CowValue{left_ref.equal(right_ref)};
  }
  case ast::BinaryOperator::NotEqual: {
    return CowValue{left_ref.not_equal(right_ref)};
  }
  case ast::BinaryOperator::Less: {
    return CowValue{left_ref.less(right_ref)};
  }
  case ast::BinaryOperator::LessEqual: {
    return CowValue{left_ref.less_equal(right_ref)};
  }
  case ast::BinaryOperator::Greater: {
    return CowValue{left_ref.greater(right_ref)};
  }
  case ast::BinaryOperator::GreaterEqual: {
    return CowValue{left_ref.greater_equal(right_ref)};
  }
  default:
    throw std::runtime_error(
        std::format("not implemented: {}", binary.get_op())
    );
  }
}

CowValue VM::evaluate(const ast::Identifier &identifier) const {
  if (auto value = read_variable(identifier)) {
    return CowValue{*value};
  }
  throw UndefinedVariableError(identifier);
}

CowValue VM::evaluate(const ast::Variable &variable) const {
  return evaluate(variable.get_identifier());
}

CowValue VM::evaluate(const ast::Literal &literal) const {
  debug_print("Evaluating literal");
  Value primitive = match::match(
      literal.get(),
      [](const ast::Nil & /*unused*/) -> Value { return {Nil{}}; },
      [](const auto &literal_value) -> Value {
        return Primitive{literal_value.get()};
      }
  );
  debug_print("Primitive: {}", primitive);

  return CowValue{std::move(primitive)};
}
CowValue VM::evaluate(const ast::Expression &expression) {
  debug_print("Evaluating expression");
  return expression.visit([this](const auto &expr) {
    auto result = evaluate(expr);
    debug_print("Expression result: {}", result.as_cref());
    return result;
  });
}
void VM::execute(const ast::Assignment &assignment) {
  const auto &variable = assignment.get_variable();
  debug_print("Executing assignment to {}", variable.get_identifier().name());
  auto value = read_write_variable(variable.get_identifier());
  if (!value) {
    throw UndefinedVariableError(variable);
  }
  const auto &expression = assignment.get_expression();
  auto &lhs = value->get();
  const auto rhs = evaluate(expression);
  const auto &rhs_ref = rhs.as_cref();
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
  debug_print("Assigned: {}", value->get());
}
void VM::execute(const ast::Declaration &declaration) {
  const auto &variable = declaration.get_variable();
  const auto &expression = declaration.get_expression();
  debug_print("Executing declaration of {}", variable.name());
  auto &value = current_scope().declare_variable(variable);
  value = evaluate(expression).into_value();
  debug_print("Declared {} = {}", variable.name(), value);
}
void VM::execute(const ast::FunctionCall &function_call) {
  debug_print("Executing function call");
  const auto &value = evaluate(function_call);
  if (!value->is_nil()) {
    throw RuntimeError("Top-value function call returned non-nil value");
  }
}
void VM::execute(const ast::IfStatement &if_statement) {
  debug_print("Evaluating if statement");
  if (evaluate_if_branch(if_statement.get_base_if()) ||
      std::ranges::any_of(
          if_statement.get_elseif(),
          std::bind_front(&VM::evaluate_if_branch, this)
      )) {
    return;
  }

  debug_print("Executing else block");
  if (auto else_block = if_statement.get_else_block(); else_block) {
    execute(else_block->get());
  }
}
CowValue VM::evaluate(const ast::FunctionCall &function_call) {
  const auto &function = function_call.get_name();
  const auto &arguments = function_call.get_arguments();

  debug_print("Calling function {}", function.get_identifier());

  const auto evaluated_function = evaluate(function);

  const Value::function_type &function_ptr = evaluated_function->visit(
      [](const Value::function_type &function) -> Value::function_type {
        return function;
      },
      [](const auto &value) -> Value::function_type {
        throw std::runtime_error(std::format("{} is not a function", value));
      }
  );

  std::vector<CowValue> evaluated_arguments;
  for (const auto &argument : arguments) {
    evaluated_arguments.push_back(evaluate(argument));
  }

  debug_print("Arguments:");
  for (const auto &argument : evaluated_arguments) {
    debug_print("  {}", argument);
  }

  return function_ptr->operator()(*this, evaluated_arguments);
};
void VM::execute(const ast::Statement &statement) {
  debug_print("Executing statement");
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
    std::println(std::cerr, "Exception: {}", e.what());
    cpptrace::from_current_exception().print();
  }
}

CowValue VM::evaluate(const ast::Block &block) {
  debug_print("Evaluating block");
  for (const auto &statement : block.get_statements()) {
    execute(statement);
  }

  if (const auto &last_statement = block.get_last_statement(); last_statement) {
    debug_print("Block has last statement");
    return evaluate(*last_statement);
  }
  return CowValue{Value{}};
}

std::optional<std::reference_wrapper<const Value>>
VM::read_variable(const ast::Identifier &id) const {
  for (const auto &it : std::views::reverse(scopes)) {
    if (auto value = it->get_variable(id)) {
      return value;
    }
  }
  return Scope::builtins().get_variable(id);
}

std::optional<std::reference_wrapper<Value>>
VM::read_write_variable(const ast::Identifier &id) {
  for (auto &it : std::views::reverse(scopes)) {
    if (auto value = it->get_variable(id)) {
      return value;
    }
  }
  return std::nullopt;
}

bool VM::evaluate_if_branch(const ast::IfBase &if_base) {
  debug_print("Evaluating if branch");
  const auto condition_value = evaluate(if_base.get_condition());
  const auto &condition = condition_value.as_cref();
  if (condition.as_bool()) {
    debug_print("Condition is truthy {}", condition_value);
    execute(if_base.get_block());
    return true;
  }

  debug_print("Condition is falsy {}", condition_value);
  return false;
};

void VM::execute(const ast::NamedFunction &named_function) {
  debug_print("Declaring named function");
  const auto &name = named_function.get_name();
  auto &variable = current_scope().declare_variable(name);

  auto function = L3Function{scopes, named_function};
  variable = Value{Function{std::move(function)}};

  debug_print("Declared function {}", name.name());
}

CowValue VM::evaluate_function_body(
    std::span<std::shared_ptr<Scope>> captured,
    Scope &&arguments,
    const ast::FunctionBody &body
) {
  debug_print("entering function body");
  for (const auto &capture : captured) {
    debug_print("captured: {}", capture->get_variables());
  }
  debug_print("arguments: {}", arguments.get_variables());

  auto original_scopes = std::move(scopes);

  scopes = {};
  for (const auto &capture : captured) {
    scopes.push_back(capture);
  }
  scopes.push_back(std::make_shared<Scope>(std::move(arguments)));

  auto return_value = evaluate(body.get_block());

  scopes = std::move(original_scopes);

  return return_value;
}

CowValue VM::evaluate(const ast::LastStatement &last_statement) {
  debug_print("Evaluating last statement");
  return last_statement.visit(
      match::Overloaded{
          [this](const ast::ReturnStatement &return_statement) {
            auto value = return_statement.get_expression()
                             .transform([this](const auto &expression) {
                               return evaluate(expression);
                             })
                             .value_or(CowValue{Value{}});
            debug_print("Returning {}", value);
            return value;
          },
          [](const auto & /*unused*/) -> CowValue {
            throw std::runtime_error("last statement not implemented");
          }
      }
  );
}

CowValue VM::evaluate(const ast::AnonymousFunction &anonymous) const {
  return CowValue{Function{L3Function{scopes, anonymous}}};
}

Scope &VM::current_scope() { return *scopes.back(); }
} // namespace l3::vm
