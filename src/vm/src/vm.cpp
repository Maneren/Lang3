#include "vm/vm.hpp"
#include "vm/error.hpp"
#include "vm/format.hpp"
#include "vm/scope.hpp"
#include "vm/value.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <print>
#include <ranges>
#include <utility>
#include <utils/match.h>
#include <vector>

namespace l3::vm {

RefValue VM::evaluate(const ast::UnaryExpression &unary) {
  debug_print("Evaluating unary expression {}", unary.get_op());
  const auto argument = evaluate(unary.get_expr());
  switch (unary.get_op()) {
  case ast::UnaryOperator::Minus: {
    return store_value(argument->negative());
  }
  case ast::UnaryOperator::Plus: {
    return argument;
  }
  case ast::UnaryOperator::Not: {
    return store_value(argument->not_op());
  }
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
  case ast::BinaryOperator::Plus: {
    return store_value(left.add(right));
  }
  case ast::BinaryOperator::Minus: {
    return store_value(left.sub(right));
  }
  case ast::BinaryOperator::Multiply: {
    return store_value(left.mul(right));
  }
  case ast::BinaryOperator::Divide: {
    return store_value(left.div(right));
  }
  case ast::BinaryOperator::Modulo: {
    return store_value(left.mod(right));
  }
  case ast::BinaryOperator::And: {
    return store_value(left.and_op(right));
  }
  case ast::BinaryOperator::Or: {
    return store_value(left.or_op(right));
  }
  case ast::BinaryOperator::Equal: {
    return store_value(left.equal(right));
  }
  case ast::BinaryOperator::NotEqual: {
    return store_value(left.not_equal(right));
  }
  case ast::BinaryOperator::Less: {
    return store_value(left.less(right));
  }
  case ast::BinaryOperator::LessEqual: {
    return store_value(left.less_equal(right));
  }
  case ast::BinaryOperator::Greater: {
    return store_value(left.greater(right));
  }
  case ast::BinaryOperator::GreaterEqual: {
    return store_value(left.greater_equal(right));
  }
  default:
    throw std::runtime_error(
        std::format("not implemented: {}", binary.get_op())
    );
  }
}

RefValue VM::evaluate(const ast::Identifier &identifier) {
  return read_variable(identifier);
}

RefValue VM::evaluate(const ast::Variable &variable) {
  return evaluate(variable.get_identifier());
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
      [](const auto &literal_value) -> Value {
        return Primitive{literal_value.get_value()};
      }
  );
  debug_print("Literal: {}", value);

  return store_value(std::move(value));
}
RefValue VM::evaluate(const ast::Expression &expression) {
  debug_print("Evaluating expression");
  return expression.visit([this](const auto &expr) {
    auto result = evaluate(expr);
    debug_print("Expression result: {}", result.get());
    return result;
  });
}
void VM::execute(const ast::OperatorAssignment &assignment) {
  const auto &variable = assignment.get_variable();
  debug_print(
      "Executing assignment to {}", variable.get_identifier().get_name()
  );
  auto &lhs = read_write_variable(variable.get_identifier()).get();
  const auto &expression = assignment.get_expression();
  const auto rhs = evaluate(expression);

  switch (assignment.get_operator()) {
  case ast::AssignmentOperator::Assign:
    lhs = rhs;
    break;
  case ast::AssignmentOperator::Plus:
    lhs = store_value(lhs->add(*rhs));
    break;
  case ast::AssignmentOperator::Minus:
    lhs = store_value(lhs->sub(*rhs));
    break;
  case ast::AssignmentOperator::Multiply:
    lhs = store_value(lhs->mul(*rhs));
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
}
void VM::execute(const ast::FunctionCall &function_call) {
  auto _ = evaluate(function_call);
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
RefValue VM::evaluate(const ast::FunctionCall &function_call) {
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

  auto evaluated_arguments =
      arguments |
      std::views::transform([this](const auto &arg) { return evaluate(arg); }) |
      std::ranges::to<std::vector>();

  debug_print("Arguments:");
  for (const auto &argument : evaluated_arguments) {
    debug_print("  {}", argument);
  }

  auto result = function_ptr->operator()(*this, evaluated_arguments);
  debug_print("Result: {}", result);
  return result;
};
void VM::execute(const ast::Statement &statement) {
  debug_print("Executing statement");
  statement.visit([this](const auto &stmt) { execute(stmt); });
}
void VM::execute(const ast::Program &program) {
  try {
    for (const auto &statement : program.get_statements()) {
      execute(statement);
    }
  } catch (const RuntimeError &error) {
    std::println(std::cerr, "{}: {}", error.type(), error.what());
  }
}

void VM::execute(const ast::Block &block) {
  debug_print("Evaluating block");
  stack.push_frame();
  scopes.push_back(std::make_shared<Scope>());
  for (const auto &statement : block.get_statements()) {
    execute(statement);
  }

  const auto &last_statement = block.get_last_statement();

  if (last_statement) {
    execute(*last_statement);
  }
  const auto created_values =
      stack.top_frame().size() + scopes.back()->get_variables().size();
  stack.pop_frame();
  scopes.pop_back();
  if (created_values > 0) {
    run_gc();
  }
}

RefValue VM::read_variable(const Identifier &id) const {
  debug_print("Reading variable {}", id.get_name());
  for (const auto &it : std::views::reverse(scopes)) {
    const auto &scope = *it;
    if (auto variable = scope.get_variable(id)) {
      return *variable->get();
    }
  }
  if (auto value = Scope::get_builtin(id)) {
    return *value;
  }
  throw UndefinedVariableError(id);
}

std::reference_wrapper<RefValue> VM::read_write_variable(const Identifier &id) {
  debug_print("Writing variable {}", id.get_name());
  for (auto &it : std::views::reverse(scopes)) {
    if (auto variable = it->get_variable_mut(id)) {
      if (variable->get().is_const()) {
        throw RuntimeError("Cannot modify const variable {}", id);
      }
      return *variable->get();
    }
  }
  if (auto value = Scope::get_builtin(id)) {
    throw RuntimeError("Cannot modify builtin variable {}", id);
  }
  throw UndefinedVariableError(id);
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
};

void VM::execute(const ast::NamedFunction &named_function) {
  debug_print("Declaring named function");
  const auto &name = named_function.get_name();
  *declare_variable(name, Mutability::Immutable) =
      store_value({Function{L3Function{scopes, named_function}}});

  debug_print("Declared function {}", name.get_name());
}

RefValue VM::evaluate_function_body(
    std::span<std::shared_ptr<Scope>> captured,
    Scope &&arguments,
    const ast::FunctionBody &body
) {
  for (const auto &capture : captured) {
    debug_print("captured: {}", capture->get_variables());
  }
  debug_print("arguments: {}", arguments.get_variables());

  unused_scopes.emplace_back(std::move(scopes));

  scopes = captured | std::ranges::to<std::vector>();
  scopes.push_back(std::make_shared<Scope>(std::move(arguments)));

  std::optional<RefValue> return_value;

  try {
    execute(body.get_block());
  } catch (const BreakFlowException &exception) {
    return_value = exception.value;
  }

  scopes = std::move(unused_scopes.back());
  unused_scopes.pop_back();

  auto value = stack.push_value(return_value.value_or(nil()));
  run_gc();
  return value;
}

void VM::execute(const ast::LastStatement &last_statement) {
  debug_print("Evaluating last statement");
  last_statement.visit(
      match::Overloaded{
          [this](const ast::ReturnStatement &return_statement) {
            const auto value = return_statement.get_expression()
                                   .transform([this](const auto &expr) {
                                     return evaluate(expr);
                                   })
                                   .value_or(VM::nil());
            debug_print("Returning {}", value);
            throw BreakFlowException(value);
          },
          [](const auto & /*expression*/) { throw BreakFlowException(); }
      }
  );
}

RefValue VM::evaluate(const ast::AnonymousFunction &anonymous) {
  return store_value(Function{L3Function{scopes, anonymous}});
}

RefValue VM::evaluate(const ast::IndexExpression &index_ex) {
  auto base = evaluate(index_ex.get_base());
  auto index = evaluate(index_ex.get_index());
  return store_new_value(base->index(*index));
}

VM::VM(bool debug)
    : debug{debug}, scopes{std::make_shared<Scope>()}, stack{debug},
      gc_storage{debug} {}

size_t VM::run_gc() {
  debug_print("Running GC");
  for (auto &scope : scopes) {
    scope->mark_gc();
  }
  for (auto &scope_group : unused_scopes) {
    for (auto &scope : scope_group) {
      scope->mark_gc();
    }
  }
  stack.mark_gc();
  auto erased = gc_storage.sweep();
  debug_print("[GC] Swept {} values", erased);
  return erased;
}

Variable &
VM::declare_variable(const Identifier &id, Mutability mut, GCValue &gc_value) {
  return scopes.back()->declare_variable(id, RefValue{gc_value}, mut);
}

RefValue VM::store_value(Value &&value) {
  auto &gc_value = gc_storage.emplace(std::move(value));
  auto ref_value = RefValue{gc_value};
  stack.push_value(ref_value);
  return ref_value;
}

RefValue VM::store_new_value(NewValue &&value) {
  return match::match(
      std::move(value),
      [this](Value &&value) { return store_value(std::move(value)); },
      [](RefValue value) { return value; }
  );
}

void assign_variables(
    std::span<std::reference_wrapper<RefValue>> variables, RefValue value
) {

  if (variables.size() == 1) {
    variables.begin()->get() = value;
    return;
  }

  const auto value_vector_opt = value->as_vector();

  if (!value_vector_opt) {
    throw ValueError("Destructuring assignment only works with vectors");
  }

  const auto &value_vector = value_vector_opt->get();

  if (value_vector.size() != variables.size()) {
    throw ValueError(
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

void VM::execute(const ast::Declaration &declaration) {
  const auto &assignment = declaration.get_name_assignment();
  const auto &names = assignment.get_names();
  debug_print(
      "Executing declaration of {}",
      std::views::transform(names, &Identifier::get_name) |
          std::ranges::to<std::vector>()
  );

  auto mutability = declaration.get_mutability();

  auto variables = std::views::transform(
                       names,
                       [&, this](const auto &id) {
                         return std::ref(*declare_variable(id, mutability));
                       }
                   ) |
                   std::ranges::to<std::vector>();

  assign_variables(variables, evaluate(assignment.get_expression()));
}

void VM::execute(const ast::NameAssignment &assignment) {
  const auto &names = assignment.get_names();
  const auto value = evaluate(assignment.get_expression());
  debug_print(
      "Executing name assignment {} = {}",
      std::views::transform(names, &Identifier::get_name) |
          std::ranges::to<std::vector>(),
      value
  );

  auto variables = std::views::transform(
                       names, std::bind_front(&VM::read_write_variable, this)
                   ) |
                   std::ranges::to<std::vector>();

  assign_variables(variables, value);
}

RefValue VM::nil() { return RefValue{GCStorage::nil()}; }

RefValue VM::evaluate(const ast::IfExpression &if_expr) {
  std::optional<RefValue> result;

  try {
    execute(static_cast<const ast::IfElseBase &>(if_expr));
    const auto &else_expr = if_expr.get_else_block();
    execute(else_expr);
  } catch (const BreakFlowException &e) {
    result = e.value;
  }

  if (result) {
    return *result;
  }

  throw RuntimeError("if expression did not return a value");
}

void VM::execute(const ast::While &while_loop) {
  const auto &condition = while_loop.get_condition();
  const auto &body = while_loop.get_body();

  while (evaluate(condition)->is_truthy()) {
    execute(body);
  }
}

} // namespace l3::vm
