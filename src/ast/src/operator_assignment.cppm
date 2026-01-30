module;

#include <iterator>
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

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Assignment {}", op);
    variable.print(out, depth + 1);
    expression.print(out, depth + 1);
  }

  DEFINE_ACCESSOR_X(variable);
  DEFINE_VALUE_ACCESSOR(operator, AssignmentOperator, op)
  DEFINE_ACCESSOR_X(expression);
};

} // namespace l3::ast
