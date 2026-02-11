module l3.ast;

import :block;
import :expression;

namespace l3::ast {

While::While(location::Location location) : location_(std::move(location)) {}
While::While(Expression &&condition, Block &&block, location::Location location)
    : condition(std::move(condition)), body(std::move(block)),
      location_(std::move(location)) {}

While::While(While &&) noexcept = default;
While &While::operator=(While &&) noexcept = default;
While::~While() = default;

} // namespace l3::ast
