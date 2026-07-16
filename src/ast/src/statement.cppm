export module l3.ast:statement;

import std;

import utils;

import l3.location;

import :assignment;
import :declaration;
import :for_loop;
import :function;
import :function_call;
import :if_else;
import :range_for_loop;
import :while_loop;

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

  [[nodiscard]] const location::Location &get_location() const;
};

} // namespace l3::ast
