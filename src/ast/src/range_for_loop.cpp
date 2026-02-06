module l3.ast;

import :block;
import :expression;
import :identifier;

namespace l3::ast {

RangeForLoop::RangeForLoop(
    Mutability mutability,
    Identifier &&variable,
    Expression &&start,
    RangeOperator range_type,
    Expression &&end,
    std::optional<Expression> &&step,
    Block &&body
)
    : variable(std::move(variable)), start(std::move(start)),
      end(std::move(end)), step(std::move(step)), body(std::move(body)),
      range_type(range_type), mutability(mutability) {}

RangeForLoop::RangeForLoop() = default;
RangeForLoop::RangeForLoop(RangeForLoop &&) noexcept = default;
RangeForLoop &RangeForLoop::operator=(RangeForLoop &&) noexcept = default;
RangeForLoop::~RangeForLoop() = default;

} // namespace l3::ast
