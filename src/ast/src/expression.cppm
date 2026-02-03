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
  Expression();

  Expression(AnonymousFunction &&function);
  Expression(BinaryExpression &&expression);
  Expression(Comparison &&comparison);
  Expression(FunctionCall &&call);
  Expression(IfExpression &&clause);
  Expression(IndexExpression &&index);
  Expression(Literal &&literal);
  Expression(LogicalExpression &&expression);
  Expression(UnaryExpression &&expression);
  Expression(Variable &&variable);

  VISIT(inner)
};

} // namespace l3::ast
