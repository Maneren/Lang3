export module l3.ast:statement;

import utils;
import :assignment;
import :declaration;
import :function_call;
import :function;
import :if_else;
import :loop;

export namespace l3::ast {

class Statement {
  std::variant<
      Declaration,
      ForLoop,
      FunctionCall,
      IfStatement,
      NameAssignment,
      NamedFunction,
      OperatorAssignment,
      RangeForLoop,
      While>
      inner;

public:
  Statement();
  Statement(const Statement &) = delete;
  Statement(Statement &&) noexcept;
  Statement &operator=(const Statement &) = delete;
  Statement &operator=(Statement &&) noexcept;
  ~Statement();

  Statement(Assignment &&assignment);
  Statement(Declaration &&declaration);
  Statement(ForLoop &&loop);
  Statement(FunctionCall &&call);
  Statement(IfStatement &&clause);
  Statement(NameAssignment &&assignment);
  Statement(NamedFunction &&function);
  Statement(OperatorAssignment &&assignment);
  Statement(RangeForLoop &&loop);
  Statement(While &&loop);

  VISIT(inner)
};

} // namespace l3::ast
