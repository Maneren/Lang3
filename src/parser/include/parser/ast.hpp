#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace l3::ast {

enum class Unary : uint8_t { Plus, Minus, Not };
enum class Binary : uint8_t {
  Plus,
  Minus,
  Multiply,
  Divide,
  Modulo,
  Power,
  Equal,
  NotEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  And,
  Or
};

struct Nil {};
struct True {};
struct False {};
struct Num {
  long long value;
};
struct String {
  std::string value;
};
using Literal = std::variant<Nil, True, False, Num, String>;


enum class Separator : uint8_t { Comma, Semicolon };

using Identifier = std::string;

struct Expression;

using ExpressionList = std::vector<Expression>;
using FieldName = std::variant<Identifier, std::shared_ptr<Expression>>;

struct AssignmentField {
  FieldName name;
  std::shared_ptr<Expression> expr;
};

using Field = std::variant<AssignmentField, Expression>;

struct FieldList {
  std::vector<Field> fields;
};

struct Table {
  FieldList fields;
};

using Var = Identifier;

using NameList = std::vector<Identifier>;

struct FunctionCall {
  Identifier name;
  std::vector<Expression> args;
};

using PrefixExpression =
    std::variant<Var, std::shared_ptr<Expression>, FunctionCall>;

struct Block;

struct FunctionBody {
  NameList parameters;
  std::shared_ptr<Block> block;
};

struct NamedFunction {
  Identifier name;
  FunctionBody body;
};

using AnonymousFunction = FunctionBody;

using Function = std::variant<AnonymousFunction, NamedFunction>;

using Arguments = ExpressionList;

struct UnaryExpression {
  Unary op;
  std::shared_ptr<Expression> expr;
};

struct BinaryExpression {
  std::shared_ptr<Expression> lhs;
  Binary op;
  std::shared_ptr<Expression> rhs;

  BinaryExpression() = default;

  BinaryExpression(Expression &&lhs, Binary op, Expression &&rhs)
      : lhs(std::make_shared<Expression>(std::move(lhs))), op(op),
        rhs(std::make_shared<Expression>(std::move(rhs))) {}

  BinaryExpression(Expression &lhs, Binary op, Expression &rhs)
      : lhs(std::make_shared<Expression>(lhs)), op(op),
        rhs(std::make_shared<Expression>(rhs)) {}
};

struct Expression {
  std::variant<
      Literal,
      UnaryExpression,
      BinaryExpression,
      PrefixExpression,
      AnonymousFunction,
      Table>
      inner;

  Expression(Literal &&literal) : inner(std::move(literal)) {}
  Expression(UnaryExpression &&expression) : inner(std::move(expression)) {}
  Expression(BinaryExpression &&expression) : inner(std::move(expression)) {}
  Expression(PrefixExpression &&expression) : inner(std::move(expression)) {}
  Expression(AnonymousFunction &&function) : inner(std::move(function)) {}
  Expression(Table &&table) : inner(std::move(table)) {}

  Expression() = default;
};

using VarList = std::vector<Var>;

struct IfClause;

// using ElseClause =
//     std::variant<std::shared_ptr<Block>, std::shared_ptr<IfClause>>;

struct IfClause {
  Expression condition;
  std::shared_ptr<Block> block;
  std::optional<std::shared_ptr<Block>> elseBlock;
};

struct Assignment {
  Var var;
  Expression expr;
};

struct Declaration {
  Identifier var;
  Expression expr;
};

using Statement = std::variant<
    Assignment,
    Declaration,
    Expression,
    FunctionCall,
    IfClause,
    NamedFunction>;

// using ReturnStatement = Expression;
//
// struct BreakStatement {};
// struct ContinueStatement {};
//
// using LastStatement =
//     std::variant<ReturnStatement, BreakStatement, ContinueStatement>;

struct Block {
  std::vector<Statement> statements;
};

} // namespace l3::ast
