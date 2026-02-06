export module l3.ast:expression_list;

export namespace l3::ast {

class Expression;
class ExpressionList : public std::deque<Expression> {
public:
  ExpressionList();
  ExpressionList(const ExpressionList &) = delete;
  ExpressionList(ExpressionList &&) noexcept;
  ExpressionList &operator=(const ExpressionList &) = delete;
  ExpressionList &operator=(ExpressionList &&) noexcept;
  ExpressionList(Expression &&expression);
  ExpressionList &with_expression(Expression &&expression);
  ~ExpressionList();
};

} // namespace l3::ast
