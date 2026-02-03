module;

#include <utility>

module l3.ast;

namespace l3::ast {

Array::Array() = default;
Array::Array(ExpressionList &&elements) : elements(std::move(elements)) {}

} // namespace l3::ast
