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

RefValue VM::evaluate(const ast::BinaryExpression &binary) {
  debug_print("Evaluating binary expression {}", binary.get_op());
  const auto left = evaluate(binary.get_lhs());
  const auto right = evaluate(binary.get_rhs());

  debug_print("Left: {}", left);
  debug_print("Right: {}", right);

  const auto &left_ref = left.get();
  const auto &right_ref = right.get();

  switch (binary.get_op()) {
  case ast::BinaryOperator::Plus: {
    return store_value(left_ref.add(right_ref));
  }
  case ast::BinaryOperator::Minus: {
    return store_value(left_ref.sub(right_ref));
  }
  case ast::BinaryOperator::Multiply: {
    return store_value(left_ref.mul(right_ref));
  }
  case ast::BinaryOperator::Divide: {
    return store_value(left_ref.div(right_ref));
  }
  case ast::BinaryOperator::Modulo: {
    return store_value(left_ref.mod(right_ref));
  }
  case ast::BinaryOperator::And: {
    return store_value(left_ref.and_op(right_ref));
  }
  case ast::BinaryOperator::Or: {
    return store_value(left_ref.or_op(right_ref));
  }
  case ast::BinaryOperator::Equal: {
    return store_value(left_ref.equal(right_ref));
  }
  case ast::BinaryOperator::NotEqual: {
    return store_value(left_ref.not_equal(right_ref));
  }
  case ast::BinaryOperator::Less: {
    return store_value(left_ref.less(right_ref));
  }
  case ast::BinaryOperator::LessEqual: {
    return store_value(left_ref.less_equal(right_ref));
  }
  case ast::BinaryOperator::Greater: {
    return store_value(left_ref.greater(right_ref));
  }
  case ast::BinaryOperator::GreaterEqual: {
    return store_value(left_ref.greater_equal(right_ref));
  }
  default:
    throw std::runtime_error(
        std::format("not implemented: {}", binary.get_op())
    );
  }
}

RefValue VM::evaluate(const ast::Identifier &identifier) {
  if (auto value = read_write_variable(identifier)) {
    return RefValue{*value};
  }
  throw UndefinedVariableError(identifier);
}

RefValue VM::evaluate(const ast::Variable &variable) {
  return evaluate(variable.get_identifier());
}

RefValue VM::evaluate(const ast::Literal &literal) {
  debug_print("Evaluating literal");
  Value value = match::match(
      literal.get(),
      [](const ast::Nil & /*unused*/) -> Value { return {Nil{}}; },
      [this](const ast::Array &array) -> Value {
        std::vector<RefValue> values;
        values.reserve(array.get().size());
        for (const auto &element : array.get()) {
          values.emplace_back(evaluate(element));
        }
        return Value{std::move(values)};
      },
      [](const auto &literal_value) -> Value {
        return Primitive{literal_value.get()};
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
  debug_print("Executing assignment to {}", variable.get_identifier().name());
  auto value = read_write_variable(variable.get_identifier());
  if (!value) {
    throw UndefinedVariableError(variable);
  }
  const auto &expression = assignment.get_expression();
  const auto rhs = evaluate(expression);

  auto &lhs = value->get();
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
  debug_print("Assigned: {}", value->get());
}
void VM::execute(const ast::Declaration &declaration) {
  const auto &names = declaration.get_names();
  const auto &variable = names[0]; // FIXME: support multiple names
  const auto &expression = declaration.get_expression();
  debug_print("Executing declaration of {}", variable.name());
  if (current_scope().has_variable(variable)) {
    throw NameError("variable '{}' already declared", variable.name());
  }
  auto value = evaluate(expression);
  current_scope().declare_variable(variable, value.get_gc());
  debug_print("Declared {} = {}", variable.name(), value);
}
void VM::execute(const ast::FunctionCall &function_call) {
  debug_print("Executing function call");
  {
    auto _ = evaluate(function_call);
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

  std::vector<RefValue> evaluated_arguments;
  for (const auto &argument : arguments) {
    evaluated_arguments.push_back(evaluate(argument));
  }

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
  CPPTRACE_TRY {
    try {
      for (const auto &statement : program.get_statements()) {
        execute(statement);
      }
    } catch (const RuntimeError &error) {
      std::println(std::cerr, "{}: {}", error.type(), error.what());
    }
  }
  CPPTRACE_CATCH(const std::exception &e) {
    std::println(std::cerr, "Exception: {}", e.what());
    cpptrace::from_current_exception().print();
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
  stack.pop_frame();
  scopes.pop_back();
}

std::optional<std::reference_wrapper<const Value>>
VM::read_variable(const ast::Identifier &id) const {
  for (const auto &it : std::views::reverse(scopes)) {
    const auto &scope = *it;
    if (auto value = scope.get_variable(id)) {
      return value;
    }
  }
  return std::nullopt;
}

std::optional<std::reference_wrapper<RefValue>>
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
  auto variable = declare_variable(name);

  auto function = L3Function{scopes, named_function};

  *variable = Value{Function{std::move(function)}};

  debug_print("Declared function {}", name.name());
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

  auto original_scopes = std::move(scopes);

  scopes = {};
  for (const auto &capture : captured) {
    scopes.push_back(capture);
  }
  scopes.push_back(std::make_shared<Scope>(std::move(arguments)));
  stack.push_frame();

  std::optional<RefValue> return_value;

  try {
    execute(body.get_block());
  } catch (const BreakFlowException &exception) {
    return_value = exception.value;
  }

  stack.pop_frame();
  scopes = std::move(original_scopes);

  auto value = stack.push_value(return_value.value_or(store_value(Value{})));
  run_gc();
  return value;
}

void VM::execute(const ast::LastStatement &last_statement) {
  debug_print("Evaluating last statement");
  last_statement.visit(
      match::Overloaded{
          [this](const ast::ReturnStatement &return_statement) {
            auto value =
                return_statement.get_expression()
                    .transform([this](const ast::Expression &expression) {
                      return evaluate(expression);
                    })
                    .value_or(store_value(Value{}));
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

[[nodiscard]] RefValue VM::evaluate(const ast::IndexExpression &index_ex) {
  auto base = evaluate(index_ex.get_base());
  auto index = evaluate(index_ex.get_index());
  return store_new_value(base->index(*index));
}

Scope &VM::current_scope() {
  if (scopes.empty()) {
    throw std::runtime_error("no current scope");
  }
  return *scopes.back();
}

VM::VM(bool debug) : debug{debug}, stack{debug}, gc_storage{debug} {
  auto builtins = Scope::builtins();

  auto &scope = scopes.emplace_back(std::make_shared<Scope>());

  for (const auto &[id, value] : builtins) {
    auto &gc_value = gc_storage.emplace(value);
    scope->declare_variable(id, gc_value);
  }
}

size_t VM::run_gc() {
  debug_print("Running GC");
  for (auto &scope : scopes) {
    scope->mark_gc();
  }
  stack.mark_gc();
  auto erased = gc_storage.sweep();
  debug_print("Erased {} values", erased);
  return erased;
}

RefValue VM::declare_variable(const ast::Identifier &id) {
  auto &gc_value = gc_storage.emplace(Value{});
  current_scope().declare_variable(id, gc_value);
  return RefValue{gc_value};
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

std::vector<RefValue> &Stack::top_frame() { return frames.back(); };
void Stack::push_frame() {
  debug_print("Pushing frame");
  frames.emplace_back();
}
void Stack::pop_frame() {
  debug_print("Popping frame");
  frames.pop_back();
}
RefValue Stack::push_value(RefValue value) {
  debug_print("Pushing value {} on stack", value);
  top_frame().push_back(value);
  return value;
}
void Stack::mark_gc() {
  for (auto &frame : frames) {
    for (auto &value : frame) {
      value.get_gc().mark();
    }
  }
}

} // namespace l3::vm
