module;

#include <variant>

#include <utils/accessor.h>

export module l3.ast:assignment;

import :expression;
import :name_list;
import :operators;
import :variable;

export namespace l3::ast {

class OperatorAssignment {
  Variable variable;
  AssignmentOperator op = AssignmentOperator::Assign;
  Expression expression;

public:
  OperatorAssignment();
  OperatorAssignment(Variable &&variable, Expression &&expression);
  OperatorAssignment(
      Variable &&variable, AssignmentOperator op, Expression &&expression
  );

  DEFINE_ACCESSOR_X(variable);
  DEFINE_VALUE_ACCESSOR(operator, AssignmentOperator, op)
  DEFINE_ACCESSOR_X(expression);
};

class NameAssignment {
  NameList names;
  Expression expression;

public:
  NameAssignment();
  NameAssignment(NameList &&names, Expression &&expression);

  DEFINE_ACCESSOR_X(names);
  DEFINE_ACCESSOR_X(expression);
};

using Assignment = std::variant<OperatorAssignment, NameAssignment>;

} // namespace l3::ast
