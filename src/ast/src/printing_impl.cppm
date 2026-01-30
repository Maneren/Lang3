module;

#include <iterator>

export module l3.ast:printing_impl;

import :block;
import :expression;
import :if_else;
import :printing;

export namespace l3::ast {

void Array::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  format_indented_line(out, depth, "Array");
  for (const auto &element : elements) {
    element.print(out, depth + 1);
  }
}

void UnaryExpression::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  format_indented_line(out, depth, "UnaryExpression {}", op);
  expression->print(out, depth + 1);
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
  for (const auto &arg : arguments) {
    arg.print(out, depth + 2);
  }
}

void FunctionBody::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  format_indented_line(out, depth, "Parameters");
  for (const Identifier &parameter : parameters) {
    parameter.print(out, depth + 1);
  }
  block->print(out, depth);
}

void IfBase::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  format_indented_line(out, depth, "Condition");
  condition->print(out, depth + 1);
  block->print(out, depth);
}

void IfStatement::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  format_indented_line(out, depth, "IfStatement");
  get_base_if().print(out, depth + 1);
  get_elseif().print(out, depth + 1);
  if (else_block) {
    format_indented_line(out, depth + 1, "Else");
    else_block.value()->print(out, depth + 2);
  }
}

void IfExpression::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  format_indented_line(out, depth, "IfExpression");
  get_base_if().print(out, depth + 1);
  get_elseif().print(out, depth + 1);
  format_indented_line(out, depth + 1, "Else");
  else_block->print(out, depth + 2);
}

void While::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  format_indented_line(out, depth, "While");
  format_indented_line(out, depth + 1, "Condition");
  get_condition().print(out, depth + 2);
  format_indented_line(out, depth + 1, "Block");
  get_body().print(out, depth + 2);
}

void ReturnStatement::print(
    std::output_iterator<char> auto &out, std::size_t depth
) const {
  format_indented_line(out, depth, "Return");
  if (expression) {
    (*expression)->print(out, depth + 1);
  }
}

} // namespace l3::ast
