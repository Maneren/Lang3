module;

#include <memory>
#include <utility>

module l3.ast;

import :block;
import :expression;
import :identifier;

namespace l3::ast {

ForLoop::ForLoop(
    Identifier &&variable,
    Expression &&collection,
    Block &&body,
    Mutability mutability
)
    : variable(std::move(variable)),
      collection(std::make_unique<Expression>(std::move(collection))),
      body(std::make_unique<Block>(std::move(body))), mutability(mutability) {}

ForLoop::ForLoop() = default;
ForLoop::ForLoop(ForLoop &&) noexcept = default;
ForLoop &ForLoop::operator=(ForLoop &&) noexcept = default;
ForLoop::~ForLoop() = default;

} // namespace l3::ast
