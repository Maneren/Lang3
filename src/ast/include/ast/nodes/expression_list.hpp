#pragma once

#include <deque>
namespace l3::ast {

class Expression;

class ExpressionList : public std::deque<Expression> {
public:
  ExpressionList() = default;
  ExpressionList(Expression &&expr);
  ExpressionList &with_expression(Expression &&);
};

} // namespace l3::ast
