#include "ast/nodes/block.hpp"
#include <memory>
#include <ranges>
#include <utility>

namespace l3::ast {

namespace {

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

} // namespace

String::String(const std::string &literal) {
  using namespace std::ranges;

  value.reserve(literal.size());

  for (auto &&[index, segment] :
       literal | views::split('\\') | views::enumerate) {
    if (index == 0) {
      // First segment has no preceding escape character
      value.append_range(segment);
    } else if (!segment.empty()) {
      value += decode_escape(segment.front());
      value.append_range(segment | views::drop(1));
    } else {
      // Do nothing if the segment is empty
    }
  }
}

UnaryExpression::UnaryExpression(UnaryOperator op, Expression &&expr)
    : op(op), expr(std::make_unique<Expression>(std::move(expr))) {}

BinaryExpression::BinaryExpression(
    Expression &&lhs, BinaryOperator op, Expression &&rhs
)
    : lhs(std::make_unique<Expression>(std::move(lhs))), op(op),
      rhs(std::make_unique<Expression>(std::move(rhs))) {}

NameList::NameList(Identifier &&ident) { emplace_front(std::move(ident)); }

NameList &NameList::with_name(Identifier &&ident) {
  emplace_front(std::move(ident));
  return *this;
}

ExpressionList::ExpressionList(Expression &&expr) {
  emplace_front(std::move(expr));
};

ExpressionList &ExpressionList::with_expression(Expression &&expr) {
  emplace_front(std::move(expr));
  return *this;
}

FunctionCall::FunctionCall(Variable &&name, ExpressionList &&args)
    : name(std::move(name)), args(std::move(args)) {}

FunctionBody::FunctionBody(NameList &&parameters, Block &&block)
    : parameters(std::move(parameters)),
      block(std::make_unique<Block>(std::move(block))) {};
FunctionBody::FunctionBody(FunctionBody &&) noexcept = default;
FunctionBody &FunctionBody::operator=(FunctionBody &&) noexcept = default;
FunctionBody::~FunctionBody() = default;

IfBase::IfBase(Expression &&condition, Block &&block)
    : condition(std::make_unique<Expression>(std::move(condition))),
      block(std::make_unique<Block>(std::move(block))) {}

IfExpression::IfExpression(IfBase &&base_if, Block &&else_block)
    : base_if(std::move(base_if)),
      else_block(std::make_unique<Block>(std::move(else_block))) {}
IfExpression::IfExpression(
    IfBase &&base_if, std::vector<IfBase> &&elseif, Block &&else_block
)
    : base_if(std::move(base_if)), elseif(std::move(elseif)),
      else_block(std::make_unique<Block>(std::move(else_block))) {}
IfExpression::IfExpression(IfExpression &&) noexcept = default;
IfExpression &IfExpression::operator=(IfExpression &&) noexcept = default;
IfExpression::~IfExpression() = default;

IfStatement::IfStatement(
    IfBase &&base_if, std::vector<IfBase> &&elseif, Block &&else_block
)
    : base_if(std::move(base_if)), elseif(std::move(elseif)),
      else_block(std::make_unique<Block>(std::move(else_block))) {};
IfStatement::IfStatement(IfBase &&base_if) : base_if(std::move(base_if)) {}
IfStatement::IfStatement(IfBase &&base_if, std::vector<IfBase> &&elseif)
    : base_if(std::move(base_if)), elseif(std::move(elseif)) {}
IfStatement::IfStatement(IfStatement &&) noexcept = default;
IfStatement &IfStatement::operator=(IfStatement &&) noexcept = default;
IfStatement::~IfStatement() = default;

Block &Block::with_statement(Statement &&statement) {
  statements.push_front(std::move(statement));
  return *this;
}

} // namespace l3::ast
