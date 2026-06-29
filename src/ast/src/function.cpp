module;

module l3.ast;

namespace l3::ast {

FunctionBody::FunctionBody(location::Location location) : location_(location) {}
FunctionBody::FunctionBody(
    NameList &&parameters, Block &&block, location::Location location
)
    : parameters(std::move(parameters)), block(std::move(block)),
      location_(location) {};

NamedFunction ::NamedFunction(location::Location location)
    : location_(location) {}
NamedFunction::NamedFunction(
    Identifier &&name, FunctionBody &&body, location::Location location
)
    : name(std::move(name)), body(std::move(body)), location_(location) {}

AnonymousFunction::AnonymousFunction(location::Location location)
    : location_(location) {}
AnonymousFunction::AnonymousFunction(
    FunctionBody &&body, location::Location location
)
    : body(std::move(body)), location_(location) {}

} // namespace l3::ast
