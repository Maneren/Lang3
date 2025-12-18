#include "parser/nodes/block.hpp"
#include "parser/nodes/detail.hpp"
#include <utility>

namespace l3::ast {

namespace detail {

char decode_escape(char c) {
  switch (c) {
  case '\\':
    return '\\';
  case 'n':
    return '\n';
  case 't':
    return '\t';
  default:
    return c;
  }
}

} // namespace detail

UnaryExpression::UnaryExpression(UnaryOperator op, Expression &&expr)
    : op(op), expr(std::make_unique<Expression>(std::move(expr))) {}

BinaryExpression::BinaryExpression(
    Expression &&lhs, BinaryOperator op, Expression &&rhs
)
    : lhs(std::make_unique<Expression>(std::move(lhs))), op(op),
      rhs(std::make_unique<Expression>(std::move(rhs))) {}

FunctionCall::FunctionCall(Identifier &&name, ExpressionList &&args)
    : name(std::move(name)), args(std::move(args)) {}

FunctionBody::FunctionBody(NameList &&parameters, Block &&block)
    : parameters(std::move(parameters)),
      block(std::make_unique<Block>(std::move(block))) {};
FunctionBody::FunctionBody(FunctionBody &&) noexcept = default;
FunctionBody &FunctionBody::operator=(FunctionBody &&) noexcept = default;
FunctionBody::~FunctionBody() = default;

IfClause::IfClause(Expression &&condition, Block &&block, Block &&elseBlock)
    : condition(std::move(condition)),
      block(std::make_unique<Block>(std::move(block))),
      elseBlock(std::make_unique<Block>(std::move(elseBlock))) {}
IfClause::IfClause(Expression &&condition, Block &&block)
    : condition(std::move(condition)),
      block(std::make_unique<Block>(std::move(block))) {};
IfClause::IfClause(IfClause &&) noexcept = default;
IfClause &IfClause::operator=(IfClause &&) noexcept = default;
IfClause::~IfClause() = default;

Block &&Block::with_statement(Statement &&statement) {
  statements.push_front(std::move(statement));
  return std::move(*this);
}

} // namespace l3::ast
