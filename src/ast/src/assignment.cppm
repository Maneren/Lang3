export module l3.ast:assignment;

import :expression;
import :name_list;
import :operators;
import :variable;
import std;
import l3.location;

export namespace l3::ast {

class OperatorAssignment {
  Variable variable;
  AssignmentOperator op = AssignmentOperator::Assign;
  Expression expression;

  DEFINE_LOCATION_FIELD()

public:
  OperatorAssignment(location::Location location = {});
  OperatorAssignment(
      Variable &&variable,
      Expression &&expression,
      location::Location location = {}
  );
  OperatorAssignment(
      Variable &&variable,
      AssignmentOperator op,
      Expression &&expression,
      location::Location location = {}
  );

  DEFINE_ACCESSOR_X(variable);
  DEFINE_VALUE_ACCESSOR(operator, AssignmentOperator, op)
  DEFINE_ACCESSOR_X(expression);
};

class NameAssignment {
  NameList names;
  Expression expression;

  DEFINE_LOCATION_FIELD()

public:
  NameAssignment(location::Location location = {});
  NameAssignment(
      NameList &&names,
      Expression &&expression,
      location::Location location = {}
  );

  DEFINE_ACCESSOR_X(names);
  DEFINE_ACCESSOR_X(expression);
};

using Assignment = std::variant<OperatorAssignment, NameAssignment>;

} // namespace l3::ast
