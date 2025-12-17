#pragma once

#include <cstdint>
#include <format>
#include <iterator>
#include <memory>
#include <optional>
#include <print>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace l3::ast {

namespace {
void indent(std::output_iterator<char> auto &out, size_t depth) {
  for (size_t i = 0; i < depth; ++i) {
    std::format_to(out, " ");
  }
}
} // namespace

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

struct Nil {
  void print(std::output_iterator<char> auto &out, size_t depth) const {
    indent(out, depth);
    std::format_to(out, "Nil\n");
  }
};
class Boolean {
  bool value;

public:
  Boolean(bool value) : value(value) {}
  void print(std::output_iterator<char> auto &out, size_t depth) const {
    indent(out, depth);
    std::format_to(out, "Boolean {}\n", value);
  }
};

class Num {
  long long value;

public:
  Num(long long value) : value(value) {}
  void print(std::output_iterator<char> auto &out, size_t depth) const {
    indent(out, depth);
    std::format_to(out, "Num {}\n", value);
  }
};

class String {
  std::string value;

public:
  String(std::string &&value) : value(std::move(value)) {}
  void print(std::output_iterator<char> auto &out, size_t depth) const {
    indent(out, depth);
    std::format_to(out, "String {}\n", value);
  }
};
class Literal {
  std::variant<Nil, Boolean, Num, String> inner;

public:
  Literal() = default;
  Literal(Nil nil) : inner(nil) {}
  Literal(Boolean boolean) : inner(boolean) {}
  Literal(Num num) : inner(num) {}
  Literal(String &&string) : inner(std::move(string)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    indent(out, depth);
    std::format_to(out, "Literal\n");
    inner.visit([&out, depth](const auto &node) -> void {
      node.print(out, depth + 1);
    });
  }
};

// enum class Separator : uint8_t { Comma, Semicolon };

using Identifier = std::string;

class Expression;

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

class Block;

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

class BinaryExpression {
  std::shared_ptr<Expression> lhs;
  Binary op;
  std::shared_ptr<Expression> rhs;

public:
  BinaryExpression() = default;

  BinaryExpression(Expression &&lhs, Binary op, Expression &&rhs)
      : lhs(std::make_shared<Expression>(std::move(lhs))), op(op),
        rhs(std::make_shared<Expression>(std::move(rhs))) {}

  BinaryExpression(Expression &lhs, Binary op, Expression &rhs)
      : lhs(std::make_shared<Expression>(lhs)), op(op),
        rhs(std::make_shared<Expression>(rhs)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

class Expression {
  std::variant<
      Literal,
      // UnaryExpression,
      BinaryExpression
      // PrefixExpression,
      // AnonymousFunction,
      // Table
      >
      inner;

public:
  Expression() = default;

  Expression(Literal &&literal) : inner(std::move(literal)) {}
  // Expression(UnaryExpression &&expression) : inner(std::move(expression)) {}
  Expression(BinaryExpression &&expression) : inner(std::move(expression)) {}
  // Expression(PrefixExpression &&expression) : inner(std::move(expression)) {}
  // Expression(AnonymousFunction &&function) : inner(std::move(function)) {}
  // Expression(Table &&table) : inner(std::move(table)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    indent(out, depth);
    std::format_to(out, "Expression\n");
    inner.visit([&out, depth](const auto &node) -> void {
      node.print(out, depth + 1);
    });
  }
};

inline void BinaryExpression::print(
    std::output_iterator<char> auto &out, size_t depth
) const {
  indent(out, depth);
  std::format_to(out, "BinaryExpression\n");
  lhs->print(out, depth + 1);
  indent(out, depth + 1);
  std::format_to(out, "{}\n", static_cast<uint8_t>(op));
  rhs->print(out, depth + 1);
}

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

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    indent(out, depth);
    std::format_to(out, "Assignment '{}'\n", var);
    expr.print(out, depth + 1);
  }
};

struct Declaration {
  Identifier var;
  Expression expr;

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    indent(out, depth);
    std::format_to(out, "Declaration '{}'\n", var);
    expr.print(out, depth + 1);
  }
};

class Statement {
  std::variant<
      // Assignment,
      Declaration,
      Expression
      // FunctionCall,
      // IfClause,
      // NamedFunction
      >
      inner;

public:
  Statement() = default;
  Statement(Declaration &&declaration) : inner(std::move(declaration)) {}
  Statement(Expression &&expression) : inner(std::move(expression)) {}
  void print(std::output_iterator<char> auto &out, size_t depth) const {
    indent(out, depth);
    std::format_to(out, "Statement\n");
    inner.visit([&out, depth](const auto &node) -> void {
      node.print(out, depth + 1);
    });
  }
};

class ReturnStatement {
  std::optional<Expression> expr;

public:
  ReturnStatement() = default;
  ReturnStatement(Expression &&expression) : expr(std::move(expression)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    indent(out, depth);
    std::format_to(out, "Return\n");
    if (expr) {
      expr->print(out, depth + 1);
    }
  }
};

struct BreakStatement {
  void print(std::output_iterator<char> auto &out, size_t depth) const {
    indent(out, depth);
    std::format_to(out, "Break\n");
  }
};

struct ContinueStatement {
  void print(std::output_iterator<char> auto &out, size_t depth) const {
    indent(out, depth);
    std::format_to(out, "Continue\n");
  }
};

class LastStatement {
  std::variant<ReturnStatement, BreakStatement, ContinueStatement> inner;

public:
  LastStatement() = default;
  LastStatement(ReturnStatement &&statement) : inner(std::move(statement)) {}
  LastStatement(BreakStatement statement) : inner(statement) {}
  LastStatement(ContinueStatement statement) : inner(statement) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    indent(out, depth);
    std::format_to(out, "LastStatement\n");
    inner.visit([&out, depth](const auto &node) -> void {
      node.print(out, depth + 1);
    });
  }
};

class Block {
  std::vector<Statement> statements;
  std::optional<LastStatement> lastStatement;

public:
  Block() = default;
  Block(std::vector<Statement> &&statements)
      : statements(std::move(statements)) {}
  Block(std::vector<Statement> &&statements, LastStatement &&lastStatement)
      : statements(std::move(statements)),
        lastStatement(std::move(lastStatement)) {}

  void add_statement(Statement &&statement);

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    indent(out, depth);
    std::format_to(out, "Block\n");
    for (const Statement &statement : statements) {
      statement.print(out, depth + 1);
    }
    if (lastStatement) {
      lastStatement->print(out, depth + 1);
    }
  }
};

using Program = Block;

} // namespace l3::ast
