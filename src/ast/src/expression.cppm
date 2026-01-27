module;

#include <memory>
#include <utils/accessor.h>
#include <utils/match.h>
#include <variant>

export module l3.ast:expression;

import :function;
import :identifier;
import :if_else;
import :literal;
import :operators;
import :printing;

export namespace l3::ast {

class Expression;

class UnaryExpression {
  UnaryOperator op = UnaryOperator::Plus;
  std::unique_ptr<Expression> expr;

public:
  UnaryExpression() = default;
  UnaryExpression(UnaryOperator op, Expression &&expr);

  void print(std::output_iterator<char> auto &out, std::size_t depth) const;

  DEFINE_VALUE_ACCESSOR(op, UnaryOperator, op)
  DEFINE_PTR_ACCESSOR(expr, Expression, expr)
};

class BinaryExpression {
  std::unique_ptr<Expression> lhs;
  BinaryOperator op = BinaryOperator::Plus;
  std::unique_ptr<Expression> rhs;

public:
  BinaryExpression() = default;
  BinaryExpression(Expression &&lhs, BinaryOperator op, Expression &&rhs);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_PTR_ACCESSOR(lhs, Expression, lhs)
  DEFINE_PTR_ACCESSOR(rhs, Expression, rhs)
  DEFINE_VALUE_ACCESSOR(op, BinaryOperator, op)
};

class IndexExpression {
  std::unique_ptr<Expression> base;
  std::unique_ptr<Expression> index;

public:
  IndexExpression();
  IndexExpression(Expression &&base, Expression &&index);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_PTR_ACCESSOR(base, Expression, base)
  DEFINE_PTR_ACCESSOR(index, Expression, index)
};

class FunctionCall {
  Variable name;
  ExpressionList args;

public:
  FunctionCall() = default;
  FunctionCall(Variable &&name, ExpressionList &&args)
      : name(std::move(name)), args(std::move(args)) {}

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_ACCESSOR(name, Variable, name)
  DEFINE_ACCESSOR(arguments, ExpressionList, args)
};

class Expression {
  std::variant<
      Literal,
      UnaryExpression,
      BinaryExpression,
      Variable,
      IndexExpression,
      FunctionCall,
      AnonymousFunction,
      IfExpression
      // Table
      >
      inner;

public:
  Expression() = default;

  Expression(Literal &&literal) : inner(std::move(literal)) {}
  Expression(UnaryExpression &&expression) : inner(std::move(expression)) {}
  Expression(BinaryExpression &&expression) : inner(std::move(expression)) {}
  Expression(Variable &&var) : inner(std::move(var)) {}
  Expression(FunctionCall &&call) : inner(std::move(call)) {}
  Expression(IndexExpression &&index) : inner(std::move(index)) {}
  Expression(AnonymousFunction &&function) : inner(std::move(function)) {}
  Expression(IfExpression &&clause) : inner(std::move(clause)) {}
  // Expression(Table &&table) : inner(std::move(table)) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    visit([&out, depth](const auto &node) -> void { node.print(out, depth); });
  }

  VISIT(inner)
};

void UnaryExpression::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  format_indented_line(out, depth, "UnaryExpression {}", op);
  expr->print(out, depth + 1);
}

void BinaryExpression::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  format_indented_line(out, depth, "BinaryExpression {}", op);
  lhs->print(out, depth + 1);
  rhs->print(out, depth + 1);
}

void IndexExpression::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  format_indented_line(out, depth, "IndexExpression");
  base->print(out, depth + 1);
  index->print(out, depth + 1);
}

void FunctionCall::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  format_indented_line(out, depth, "FunctionCall");
  name.print(out, depth + 1);
  format_indented_line(out, depth + 1, "Arguments");
  for (const auto &arg : args) {
    arg.print(out, depth + 2);
  }
}

} // namespace l3::ast
