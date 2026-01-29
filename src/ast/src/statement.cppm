module;

#include <iterator>
#include <utils/accessor.h>
#include <utils/match.h>
#include <variant>

export module l3.ast:statement;

import :expression;
import :function;
import :identifier;
import :if_else;
import :mutability;
import :operators;
import :printing;
import :loop;

export namespace l3::ast {

class OperatorAssignment {
  Variable var;
  AssignmentOperator op = AssignmentOperator::Assign;
  Expression expr;

public:
  OperatorAssignment() = default;
  OperatorAssignment(Variable &&var, AssignmentOperator op, Expression &&expr)
      : var(std::move(var)), op(op), expr(std::move(expr)) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Assignment {}", op);
    var.print(out, depth + 1);
    expr.print(out, depth + 1);
  }

  DEFINE_ACCESSOR(variable, Variable, var)
  DEFINE_VALUE_ACCESSOR(operator, AssignmentOperator, op)
  DEFINE_ACCESSOR(expression, Expression, expr)
};

class NameAssignment {
  NameList names;
  Expression expr;

public:
  NameAssignment() = default;
  NameAssignment(NameList &&names, Expression &&expr)
      : names(std::move(names)), expr(std::move(expr)) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Assignment");
    format_indented_line(out, depth + 1, "Names");
    get_names().print(out, depth + 2);
    format_indented_line(out, depth + 1, "Expression");
    get_expression().print(out, depth + 2);
  }

  DEFINE_ACCESSOR(names, NameList, names)
  DEFINE_ACCESSOR(expression, Expression, expr)
};

using Assignment = std::variant<OperatorAssignment, NameAssignment>;

class Declaration {
  NameAssignment name_assignment;
  Mutability mut = Mutability::Immutable;

public:
  Declaration() = default;
  Declaration(NameAssignment &&name_assignment, Mutability mut)
      : name_assignment(std::move(name_assignment)), mut(mut) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Declaration");
    get_name_assignment().print(out, depth + 1);
  }

  DEFINE_ACCESSOR(name_assignment, NameAssignment, name_assignment)
  DEFINE_VALUE_ACCESSOR(mutability, Mutability, mut)
  [[nodiscard]] bool is_const() const { return mut == Mutability::Immutable; }
  [[nodiscard]] bool is_mutable() const { return mut == Mutability::Mutable; }
};

class Statement {
  std::variant<
      Declaration,
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
