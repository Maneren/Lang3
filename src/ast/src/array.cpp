module l3.ast;

namespace l3::ast {

Array::Array(location::Location location) : location_(location) {}
Array::Array(ExpressionList &&elements, location::Location location)
    : elements(std::move(elements)), location_(location) {}

} // namespace l3::ast
