module l3.ast;

import utils;
import :expression;

namespace l3::ast {

IfBase::IfBase(Expression &&condition, Block &&block)
    : condition(std::make_unique<Expression>(std::move(condition))),
      block(std::move(block)) {}

ElseIfList &ElseIfList::with_if(IfBase &&if_base) {
  inner.push_back(std::move(if_base));
  return *this;
}

IfElseBase::~IfElseBase() = default;
IfElseBase::IfElseBase(IfBase &&base_if) : base_if(std::move(base_if)) {};
IfElseBase::IfElseBase(IfBase &&base_if, ElseIfList &&elseif)
    : base_if(std::move(base_if)), elseif(std::move(elseif)) {};

IfExpression::IfExpression(IfBase &&base_if, Block &&else_block)
    : IfElseBase(std::move(base_if)), else_block(std::move(else_block)) {}
IfExpression::IfExpression(
    IfBase &&base_if, ElseIfList &&elseif, Block &&else_block
)
    : IfElseBase(std::move(base_if), std::move(elseif)),
      else_block(std::move(else_block)) {}
IfExpression::IfExpression(IfExpression &&) noexcept = default;
IfExpression &IfExpression::operator=(IfExpression &&) noexcept = default;
IfExpression::~IfExpression() = default;

IfStatement::IfStatement(
    IfBase &&base_if, ElseIfList &&elseif, Block &&else_block
)
    : IfElseBase(std::move(base_if), std::move(elseif)),
      else_block(std::move(else_block)) {};
IfStatement::IfStatement(IfBase &&base_if) : IfElseBase(std::move(base_if)) {}
IfStatement::IfStatement(IfBase &&base_if, ElseIfList &&elseif)
    : IfElseBase(std::move(base_if), std::move(elseif)) {}
IfStatement::IfStatement(IfStatement &&) noexcept = default;
IfStatement &IfStatement::operator=(IfStatement &&) noexcept = default;
IfStatement::~IfStatement() = default;

utils::optional_cref<Block> IfStatement::get_else_block() const {
  return else_block.transform([](const auto &block) {
    return std::cref(block);
  });
}
utils::optional_ref<Block> IfStatement::get_else_block_mut() {
  return else_block.transform([](auto &block) { return std::ref(block); });
}

} // namespace l3::ast
