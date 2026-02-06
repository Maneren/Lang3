module;

module l3.ast;

namespace l3::ast {

FunctionBody::FunctionBody() = default;
FunctionBody::FunctionBody(NameList &&parameters, Block &&block)
    : parameters(std::move(parameters)), block(std::move(block)) {};

NamedFunction ::NamedFunction() = default;
NamedFunction::NamedFunction(Identifier &&name, FunctionBody &&body)
    : name(std::move(name)), body(std::move(body)) {}

AnonymousFunction::AnonymousFunction() = default;
AnonymousFunction::AnonymousFunction(FunctionBody &&body)
    : body(std::move(body)) {}

} // namespace l3::ast
