module;

#include <memory>
#include <utility>

module l3.ast;

import :block;
import :expression;

namespace l3::ast {

While::While(Expression &&condition, Block &&block)
    : condition(std::move(condition)), body(std::move(block)) {}

While::While() = default;
While::While(While &&) noexcept = default;
While &While::operator=(While &&) noexcept = default;
While::~While() = default;

} // namespace l3::ast
