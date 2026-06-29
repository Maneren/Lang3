module l3.ast;

namespace l3::ast {

FunctionCall::FunctionCall(location::Location location) : location_(location) {}
FunctionCall::FunctionCall(
    Identifier &&name, ExpressionList &&args, location::Location location
)
    : name(std::move(name)), arguments(std::move(args)), location_(location) {}

} // namespace l3::ast
