module l3.ast;

namespace l3::ast {

FunctionCall::FunctionCall() = default;
FunctionCall::FunctionCall(Identifier &&name, ExpressionList &&args)
    : name(std::move(name)), arguments(std::move(args)) {}

} // namespace l3::ast
