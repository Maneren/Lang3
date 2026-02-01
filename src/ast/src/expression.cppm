module;

#include <utils/visit.h>
#include <variant>

export module l3.ast:expression;

export import :expression_list;
export import :expression_parts;
export import :function_call;
import :function;
import :identifier;
import :if_else;
import :literal;
import :variable;

export namespace l3::ast {

class Expression {
  std::variant<
      AnonymousFunction,
      BinaryExpression,
      Comparison,
      FunctionCall,
      IfExpression,
      Literal,
      LogicalExpression,
      UnaryExpression,
      Variable>
      inner;

public:
  Expression() = default;

  Expression(AnonymousFunction &&function) : inner(std::move(function)) {}
  Expression(BinaryExpression &&expression) : inner(std::move(expression)) {}
  Expression(Comparison &&comparison) : inner(std::move(comparison)) {}
  Expression(FunctionCall &&call) : inner(std::move(call)) {}
  Expression(IfExpression &&clause) : inner(std::move(clause)) {}
  Expression(IndexExpression &&index) : inner(std::move(index)) {}
  Expression(Literal &&literal) : inner(std::move(literal)) {}
  Expression(LogicalExpression &&expression) : inner(std::move(expression)) {}
  Expression(UnaryExpression &&expression) : inner(std::move(expression)) {}
  Expression(Variable &&variable) : inner(std::move(variable)) {}

  VISIT(inner)
};

} // namespace l3::ast
