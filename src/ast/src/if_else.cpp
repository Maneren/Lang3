module l3.ast;

import utils;

namespace l3::ast {

IfBase::IfBase(
    Expression &&condition, Block &&block, location::Location location
)
    : condition(std::make_unique<Expression>(std::move(condition))),
      block(std::move(block)), location_(location) {}

ElseIfList &ElseIfList::with_if(IfBase &&if_base) {
  inner.push_back(std::move(if_base));
  return *this;
}

IfElseBase::~IfElseBase() = default;
IfElseBase::IfElseBase(IfBase &&base_if, location::Location location)
    : base_if(std::move(base_if)), location_(location) {};
IfElseBase::IfElseBase(
    IfBase &&base_if, ElseIfList &&elseif, location::Location location
)
    : base_if(std::move(base_if)), elseif(std::move(elseif)),
      location_(location) {};

IfExpression::IfExpression(
    IfBase &&base_if, Block &&else_block, location::Location location
)
    : IfElseBase(std::move(base_if), location),
      else_block(std::move(else_block)) {}
IfExpression::IfExpression(
    IfBase &&base_if,
    ElseIfList &&elseif,
    Block &&else_block,
    location::Location location
)
    : IfElseBase(std::move(base_if), std::move(elseif), location),
      else_block(std::move(else_block)) {}
IfExpression::IfExpression(IfExpression &&) noexcept = default;
IfExpression &IfExpression::operator=(IfExpression &&) noexcept = default;
IfExpression::~IfExpression() = default;

IfStatement::IfStatement(
    IfBase &&base_if, ElseIfList &&elseif, location::Location location
)
    : IfElseBase(std::move(base_if), std::move(elseif), location) {};
IfStatement::IfStatement(
    IfBase &&base_if,
    ElseIfList &&elseif,
    Block &&else_block,
    location::Location location
)
    : IfElseBase(std::move(base_if), std::move(elseif), location),
      else_block(std::move(else_block)) {}
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
