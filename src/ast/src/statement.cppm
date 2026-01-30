module;

#include <iterator>
#include <utils/visit.h>
#include <variant>

export module l3.ast:statement;

import :assignment;
import :declaration;
import :function_call;
import :function;
import :if_else;
import :printing;
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
      While>
      inner;

public:
  Statement() = default;
  Statement(const Statement &) = delete;
  Statement(Statement &&) = default;
  Statement &operator=(const Statement &) = delete;
  Statement &operator=(Statement &&) = default;
  ~Statement() = default;

  Statement(Assignment &&assignment) {
    match::match(std::move(assignment), [this](auto &&assignment) {
      inner = std::forward<decltype(assignment)>(assignment);
    });
  }
  Statement(Declaration &&declaration) : inner(std::move(declaration)) {}
  Statement(ForLoop &&loop) : inner(std::move(loop)) {}
  Statement(FunctionCall &&call) : inner(std::move(call)) {}
  Statement(IfStatement &&clause) : inner(std::move(clause)) {}
  Statement(NameAssignment &&assignment) : inner(std::move(assignment)) {}
  Statement(NamedFunction &&function) : inner(std::move(function)) {}
  Statement(OperatorAssignment &&assignment) : inner(std::move(assignment)) {}
  Statement(While &&loop) : inner(std::move(loop)) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    visit([&out, depth](const auto &node) -> void { node.print(out, depth); });
  }

  VISIT(inner)
};

} // namespace l3::ast
