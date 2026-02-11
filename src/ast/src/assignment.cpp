module l3.ast;

namespace l3::ast {

NameAssignment::NameAssignment(location::Location location)
    : location_(std::move(location)) {}
NameAssignment::NameAssignment(
    NameList &&names, Expression &&expression, location::Location location
)
    : names(std::move(names)), expression(std::move(expression)),
      location_(std::move(location)) {}

OperatorAssignment::OperatorAssignment(location::Location location)
    : location_(std::move(location)) {}
OperatorAssignment::OperatorAssignment(
    Variable &&variable, Expression &&expression, location::Location location
)
    : variable(std::move(variable)), expression(std::move(expression)),
      location_(std::move(location)) {}

OperatorAssignment::OperatorAssignment(
    Variable &&variable,
    AssignmentOperator op,
    Expression &&expression,
    location::Location location
)
    : variable(std::move(variable)), op(op), expression(std::move(expression)),
      location_(std::move(location)) {}

} // namespace l3::ast
