#include "ast/ast.hpp"
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

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

Boolean::Boolean(bool value) : value(value) {}
[[nodiscard]] const bool &Boolean::get() const { return value; }
bool &Boolean::get() { return value; }
Number::Number(std::int64_t value) : value(value) {}
[[nodiscard]] const std::int64_t &Number::get() const { return value; }
std::int64_t &Number::get() { return value; }
Float::Float(double value) : value(value) {}
[[nodiscard]] const double &Float::get() const { return value; }
double &Float::get() { return value; }
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
[[nodiscard]] const std::string &String::get() const { return value; }
std::string &String::get() { return value; }
Literal::Literal(Nil nil) : inner(nil) {}
Literal::Literal(Boolean boolean) : inner(boolean) {}
Literal::Literal(Number num) : inner(num) {}
Literal::Literal(Float num) : inner(num) {}
Literal::Literal(String &&string) : inner(std::move(string)) {}
Literal::Literal(Array &&array) : inner(std::move(array)) {}

UnaryExpression::UnaryExpression(UnaryOperator op, Expression &&expr)
    : op(op), expr(std::make_unique<Expression>(std::move(expr))) {}

BinaryExpression::BinaryExpression(
    Expression &&lhs, BinaryOperator op, Expression &&rhs
)
    : lhs(std::make_unique<Expression>(std::move(lhs))), op(op),
      rhs(std::make_unique<Expression>(std::move(rhs))) {}

Identifier::Identifier(std::string &&id) : id(std::move(id)) {}
Identifier::Identifier(std::string_view id) : id(id) {}
[[nodiscard]] const std::string &Identifier::get_name() const { return id; }
[[nodiscard]] const Identifier &Variable::get_identifier() const { return id; }
Variable::Variable(Identifier &&id) : id(std::move(id)) {}

NameList::NameList(Identifier &&ident) { emplace_front(std::move(ident)); }

NameList &NameList::with_name(Identifier &&ident) {
  emplace_front(std::move(ident));
  return *this;
}

IndexExpression::IndexExpression() = default;
IndexExpression::IndexExpression(Expression &&base, Expression &&index)
    : base(std::make_unique<Expression>(std::move(base))),
      index(std::make_unique<Expression>(std::move(index))) {};

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
FunctionBody::FunctionBody(
    NameList &&parameters, std::shared_ptr<Block> &&block
)
    : parameters(std::move(parameters)), block(std::move(block)) {};
FunctionBody::FunctionBody(FunctionBody &&) noexcept = default;
FunctionBody &FunctionBody::operator=(FunctionBody &&) noexcept = default;
FunctionBody::~FunctionBody() = default;

NamedFunction::NamedFunction(Identifier &&name, FunctionBody &&body)
    : name(std::move(name)), body(std::move(body)) {}

IfBase::IfBase(Expression &&condition, Block &&block)
    : condition(std::make_unique<Expression>(std::move(condition))),
      block(std::make_unique<Block>(std::move(block))) {}

IfElseBase::~IfElseBase() = default;
IfElseBase::IfElseBase(IfBase &&base_if) : base_if(std::move(base_if)) {};
IfElseBase::IfElseBase(IfBase &&base_if, ElseIfList &&elseif)
    : base_if(std::move(base_if)), elseif(std::move(elseif)) {};

IfExpression::IfExpression(IfBase &&base_if, Block &&else_block)
    : IfElseBase(std::move(base_if)),
      else_block(std::make_unique<Block>(std::move(else_block))) {}
IfExpression::IfExpression(
    IfBase &&base_if, std::vector<IfBase> &&elseif, Block &&else_block
)
    : IfElseBase(std::move(base_if), std::move(elseif)),
      else_block(std::make_unique<Block>(std::move(else_block))) {}
IfExpression::IfExpression(IfExpression &&) noexcept = default;
IfExpression &IfExpression::operator=(IfExpression &&) noexcept = default;
IfExpression::~IfExpression() = default;

IfStatement::IfStatement(
    IfBase &&base_if, ElseIfList &&elseif, Block &&else_block
)
    : IfElseBase(std::move(base_if), std::move(elseif)),
      else_block(std::make_unique<Block>(std::move(else_block))) {};
IfStatement::IfStatement(IfBase &&base_if) : IfElseBase(std::move(base_if)) {}
IfStatement::IfStatement(IfBase &&base_if, ElseIfList &&elseif)
    : IfElseBase(std::move(base_if), std::move(elseif)) {}
IfStatement::IfStatement(IfStatement &&) noexcept = default;
IfStatement &IfStatement::operator=(IfStatement &&) noexcept = default;
IfStatement::~IfStatement() = default;

Block::Block(LastStatement &&lastStatement)
    : lastStatement(std::move(lastStatement)) {}
Block::Block(Statement &&statement) {
  statements.emplace_front(std::move(statement));
}

Block &Block::with_statement(Statement &&statement) {
  statements.push_front(std::move(statement));
  return *this;
}

utils::optional_cref<Block> IfStatement::get_else_block() const {
  return else_block.transform([](const auto &block) {
    return std::cref(*block);
  });
}

Array::Array() = default;
Array::Array(ExpressionList &&elements) : elements(std::move(elements)) {}
const ExpressionList &Array::get() const { return elements; }

} // namespace l3::ast
