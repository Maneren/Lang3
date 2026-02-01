module;

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <print>
#include <ranges>
#include <vector>

module l3.vm;

import utils;
import l3.ast;
import :error;
import :formatting;
import :identifier;
import :ref_value;
import :scope;
import :stack;
import :storage;
import :value;
import :variable;

namespace l3::vm {

constexpr size_t GC_OBJECT_TRIGGER_TRESHOLD = 10000;

RefValue VM::evaluate(const ast::UnaryExpression &unary) {
  debug_print("Evaluating unary expression {}", unary.get_op());
  const auto argument = evaluate(unary.get_expression());
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
void VM::execute(const ast::OperatorAssignment &assignment) {
  const auto &variable = assignment.get_variable();
  debug_print("Executing assignment to {}", variable);
  auto &lhs = evaluate_mut(variable);
  const auto &expression = assignment.get_expression();
  const auto rhs = evaluate(expression);

  switch (assignment.get_operator()) {
  case ast::AssignmentOperator::Assign:
    lhs = store_value(rhs->clone());
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

  try {
    auto result = function_ptr->operator()(*this, evaluated_arguments);
    debug_print("Result: {}", result);
    return result;
  } catch (const LoopFlowException &error) {
    throw RuntimeError("Unexpected {} outside of loop", error.type());
  }
};
void VM::execute(const ast::Statement &statement) {
  debug_print("Executing statement");
  statement.visit([this](const auto &stmt) { execute(stmt); });
  run_gc();
}
void VM::execute(const ast::Program &program) {
  try {
    execute(static_cast<const ast::Block &>(program));
  } catch (const RuntimeError &error) {
    std::println(std::cerr, "{}: {}", error.type(), error.what());
  } catch (const ReturnException &error) {
    std::println(std::cerr, "Unexpected return outside of function");
  }
}

void VM::execute(const ast::Block &block) {
  debug_print("Evaluating block");
  const auto frame_guard = stack.with_frame();
  const auto scope_guard = scopes->with_frame();
  for (const auto &statement : block.get_statements()) {
    execute(statement);
  }

  const auto &last_statement = block.get_last_statement();

  if (last_statement) {
    execute(*last_statement);
  }
}

RefValue VM::read_variable(const Identifier &id) {
  debug_print("Reading variable {}", id.get_name());
  auto value = scopes->read_variable(id)
                   .transform([this](const auto &variable) {
                     return store_value(variable->clone());
                   })
                   .or_else([&id] { return Scope::get_builtin(id); });
  if (value) {
    return *value;
  }
  throw UndefinedVariableError(id);
}

RefValue &VM::read_write_variable(const Identifier &id) {
  debug_print("Writing variable {}", id.get_name());
  if (auto value = scopes->read_variable_mut(id)) {
    return *value;
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
  declare_variable(
      name,
      Mutability::Immutable,
      store_value(Function{L3Function{scopes, named_function}})
  );

  debug_print("Declared function {}", name.get_name());
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
  std::optional<RefValue> return_value;

  {
    const auto scope_guard = scopes->with_frame(std::move(arguments));

    try {
      execute(body.get_block());
    } catch (const ReturnException &exception) {
      return_value = exception.get_value();
    } catch (const LoopFlowException &exception) {
      throw RuntimeError("Unexpected {} outside of loop", exception.type());
    }
  }

  return stack.push_value(return_value.value_or(nil()));
}

void VM::execute(const ast::LastStatement &last_statement) {
  debug_print("Evaluating last statement");
  last_statement.visit(
      match::Overloaded{
          [this](const ast::ReturnStatement &return_statement) {
            const auto value = return_statement.get_expression()
                                   .transform([this](const auto &expression) {
                                     return evaluate(expression);
                                   })
                                   .value_or(VM::nil());
            debug_print("Returning {}", value);
            throw ReturnException(value);
          },
          [](const ast::BreakStatement & /*expression*/) {
            throw BreakLoopException();
          },
          [](const ast::ContinueStatement & /*expression*/) {
            throw ContinueLoopException();
          }
      }
  );
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

VM::VM(bool debug)
    : debug{debug}, scopes{std::make_shared<ScopeStack>()}, stack{debug},
      gc_storage{debug} {}

size_t VM::run_gc() {
  const auto since_last_sweep = gc_storage.get_added_since_last_sweep();
  if (since_last_sweep < GC_OBJECT_TRIGGER_TRESHOLD) {
    debug_print("[GC] Skipping... only {} values", since_last_sweep);
    return 0;
  }

  debug_print("[GC] Running");
  for (const auto &scope : scopes->get_scopes()) {
    scope->mark_gc();
  }
  for (auto &scope_stack : unused_scopes) {
    for (const auto &scope : scope_stack->get_scopes()) {
      scope->mark_gc();
    }
  }
  stack.mark_gc();
  auto erased = gc_storage.sweep();
  if (debug) {
    debug_print(
        "[GC] Swept {} values, keeping {}", erased, gc_storage.get_size()
    );
    debug_print("Stack:");
    for (const auto &frame : stack.get_frames()) {
      debug_print("  {}", frame.size());
    }
    debug_print("Scopes:");
    for (const auto &scope : scopes->get_scopes()) {
      debug_print("  {}", scope->get_variables().size());
    }
  }
  return erased;
}

Variable &VM::declare_variable(
    const Identifier &id, Mutability mutability, RefValue ref_value
) {
  return scopes->top().declare_variable(id, ref_value, mutability);
}

RefValue VM::store_value(Value &&value) {
  if (auto boolean = value.as_primitive().and_then(&Primitive::as_bool)) {
    if (*boolean) {
      return _true();
    }
    return _false();
  }

  auto ref_value = RefValue{gc_storage.emplace(std::move(value))};
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

void VM::execute(const ast::Declaration &declaration) {
  const auto &names = declaration.get_names();
  debug_print(
      "Executing declaration of {}",
      std::views::transform(names, &ast::Identifier::get_name) |
          std::ranges::to<std::vector>()
  );

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
    declare_variable(names.front(), mutability, value);
    return;
  }

  const auto value_vector_opt = value->as_vector();
  if (!value_vector_opt) {
    throw ValueError("Destructuring declaration only works with vectors");
  }
  const auto &value_vector = value_vector_opt->get();

  if (value_vector.size() != names.size()) {
    throw ValueError(
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

RefValue VM::nil() { return RefValue{GCStorage::nil()}; }
RefValue VM::_true() { return RefValue{GCStorage::_true()}; }
RefValue VM::_false() { return RefValue{GCStorage::_false()}; }

RefValue VM::evaluate(const ast::IfExpression &if_expr) {
  std::optional<RefValue> result;

  try {
    execute(static_cast<const ast::IfElseBase &>(if_expr));
    const auto &else_expr = if_expr.get_else_block();
    execute(else_expr);
  } catch (const ReturnException &e) {
    result = e.get_value();
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
    try {
      execute(body);
    } catch (const BreakLoopException &e) {
      break;
    } catch (const ContinueLoopException &e) {
      continue;
    }
  }
}

void VM::execute(const ast::ForLoop &for_loop) {
  const auto &variable = for_loop.get_variable();
  const auto &collection = for_loop.get_collection();
  const auto &body = for_loop.get_body();
  const auto mutability = for_loop.get_mutability();

  auto collection_value = evaluate(collection);

  const auto scope_guard = scopes->with_frame();
  const auto frame_guard = stack.with_frame();
  auto &value = declare_variable(variable, mutability);
  if (auto vector_opt = collection_value->as_vector()) {
    const auto &vector = vector_opt->get();
    for (const auto &item : vector) {
      *value = item;
      try {
        execute(body);
      } catch (const BreakLoopException &e) {
        break;
      } catch (const ContinueLoopException &e) {
        continue;
      }
    }
    return;
  }

  if (auto primitive_opt = collection_value->as_primitive()) {
    if (auto string_opt = primitive_opt->get().as_string()) {
      const auto &string = string_opt->get();
      for (char c : string) {
        *value = store_value(Primitive{std::string{c}});
        try {
          execute(body);
        } catch (const BreakLoopException &e) {
          break;
        } catch (const ContinueLoopException &e) {
          continue;
        }
      }
      return;
    }
  }

  throw TypeError(
      "cannot iterate over value of type '{}'", collection_value->type_name()
  );
}

} // namespace l3::vm
