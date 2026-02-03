module;

#include <utility>

module l3.ast;

namespace l3::ast {

NameAssignment::NameAssignment() = default;
NameAssignment::NameAssignment(NameList &&names, Expression &&expression)
    : names(std::move(names)), expression(std::move(expression)) {}

OperatorAssignment::OperatorAssignment() = default;
OperatorAssignment::OperatorAssignment(
    Variable &&variable, Expression &&expression
)
    : variable(std::move(variable)), expression(std::move(expression)) {}

OperatorAssignment::OperatorAssignment(
    Variable &&variable, AssignmentOperator op, Expression &&expression
)
    : variable(std::move(variable)), op(op), expression(std::move(expression)) {
}

} // namespace l3::ast
