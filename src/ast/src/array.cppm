module;

#include <deque>
#include <iterator>
#include <utils/accessor.h>

export module l3.ast:array;

export namespace l3::ast {

class Expression;
class ExpressionList : public std::deque<Expression> {
public:
  ExpressionList();
  ExpressionList(const ExpressionList &) = delete;
  ExpressionList(ExpressionList &&) noexcept;
  ExpressionList &operator=(const ExpressionList &) = delete;
  ExpressionList &operator=(ExpressionList &&) noexcept;
  ExpressionList(Expression &&expr);
  ExpressionList &with_expression(Expression &&expr);
  ~ExpressionList();
};

class Array {
  ExpressionList elements;

public:
  Array() = default;
  Array(ExpressionList &&elements) : elements(std::move(elements)) {}

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_ACCESSOR(elements, ExpressionList, elements)
};

} // namespace l3::ast
