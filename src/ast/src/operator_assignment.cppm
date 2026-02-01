module;

#include <utility>
#include <utils/accessor.h>

export module l3.ast:operator_assignment;

import :expression;
import :operators;
import :variable;

export namespace l3::ast {

class OperatorAssignment {
  Variable variable;
  AssignmentOperator op = AssignmentOperator::Assign;
  Expression expression;

public:
  OperatorAssignment() = default;
  OperatorAssignment(Variable &&variable, Expression &&expression)
      : variable(std::move(variable)), expression(std::move(expression)) {}
  OperatorAssignment(
      Variable &&variable, AssignmentOperator op, Expression &&expression
  )
      : variable(std::move(variable)), op(op),
        expression(std::move(expression)) {}

  DEFINE_ACCESSOR_X(variable);
  DEFINE_VALUE_ACCESSOR(operator, AssignmentOperator, op)
  DEFINE_ACCESSOR_X(expression);
};

} // namespace l3::ast
