module l3.ast;

import :block;
import :expression;
import :identifier;

namespace l3::ast {

ForLoop::ForLoop(location::Location location)
    : location_(std::move(location)) {}
ForLoop::ForLoop(
    Identifier &&variable,
    Expression &&collection,
    Block &&body,
    Mutability mutability,
    location::Location location
)
    : variable(std::move(variable)), collection(std::move(collection)),
      body(std::move(body)), mutability(mutability),
      location_(std::move(location)) {}

ForLoop::ForLoop(ForLoop &&) noexcept = default;
ForLoop &ForLoop::operator=(ForLoop &&) noexcept = default;
ForLoop::~ForLoop() = default;

} // namespace l3::ast
