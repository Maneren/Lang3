#include "vm/vm.hpp"
#include <iostream>

namespace l3::vm {

CowValue VM::evaluate(const ast::BinaryExpression &binary) {
  std::println(std::cerr, "Evaluating binary expression {}", binary.get_op());
  const auto left = evaluate(binary.get_lhs());
  const auto right = evaluate(binary.get_rhs());

  std::println(std::cerr, "Left: {}", *left.as_ref());
  std::println(std::cerr, "Right: {}", *right.as_ref());

  switch (binary.get_op()) {
  case ast::BinaryOperator::Plus: {
    return CowValue{left.as_ref()->add(*right.as_ref())};
  }
  case ast::BinaryOperator::Minus: {
    return CowValue{left.as_ref()->sub(*right.as_ref())};
  }
  case ast::BinaryOperator::Multiply: {
    return CowValue{left.as_ref()->mul(*right.as_ref())};
  }
  case ast::BinaryOperator::Divide: {
    return CowValue{left.as_ref()->div(*right.as_ref())};
  }
  case ast::BinaryOperator::Modulo: {
    return CowValue{left.as_ref()->mod(*right.as_ref())};
  }
  case ast::BinaryOperator::And: {
    return CowValue{left.as_ref()->and_op(*right.as_ref())};
  }
  case ast::BinaryOperator::Or: {
    return CowValue{left.as_ref()->or_op(*right.as_ref())};
  }
  default:
    throw std::runtime_error(
        std::format("not implemented: {}", binary.get_op())
    );
  }
}

CowValue VM::evaluate(const ast::Identifier &identifier) {
  if (auto value = get_variable(identifier)) {
    return *std::move(value);
  }
  throw RuntimeError("variable '{}' not declared", identifier.name());
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
    std::println(std::cerr, "Result: {}", *result.as_ref());
    return result;
  });
}
void VM::execute(const ast::Declaration &declaration) {
  std::println(std::cerr, "Executing declaration");
  const auto value = [this, &expression = declaration.get_expression()]() {
    return evaluate(expression);
  };
  current_scope().declare_variable(declaration.get_variable(), value);
}
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
} // namespace l3::vm
