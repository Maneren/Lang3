module l3.ast;

namespace l3::ast {

FunctionCall::FunctionCall(location::Location location)
    : location_(std::move(location)) {}
FunctionCall::FunctionCall(
    Identifier &&name, ExpressionList &&args, location::Location location
)
    : name(std::move(name)), arguments(std::move(args)),
      location_(std::move(location)) {}

} // namespace l3::ast
