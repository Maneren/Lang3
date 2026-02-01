module;

#include <memory>
#include <optional>
#include <utility>

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
    : variable(std::move(variable)),
      start(std::make_unique<Expression>(std::move(start))),
      end(std::make_unique<Expression>(std::move(end))),
      body(std::make_unique<Block>(std::move(body))), range_type(range_type),
      mutability(mutability) {
  if (step.has_value()) {
    this->step = std::make_unique<Expression>(std::move(*step));
  }
}

RangeForLoop::RangeForLoop() = default;
RangeForLoop::RangeForLoop(RangeForLoop &&) noexcept = default;
RangeForLoop &RangeForLoop::operator=(RangeForLoop &&) noexcept = default;
RangeForLoop::~RangeForLoop() = default;

} // namespace l3::ast
