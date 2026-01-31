module;

#include <iterator>
#include <utility>
#include <utils/accessor.h>

export module l3.ast:name_assignment;

import :expression;
import :name_list;
import :printing;

export namespace l3::ast {

class NameAssignment {
  NameList names;
  Expression expression;

public:
  NameAssignment() = default;
  NameAssignment(NameList &&names, Expression &&expression)
      : names(std::move(names)), expression(std::move(expression)) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Assignment");
    format_indented_line(out, depth + 1, "Names");
    get_names().print(out, depth + 2);
    format_indented_line(out, depth + 1, "Expression");
    get_expression().print(out, depth + 2);
  }

  DEFINE_ACCESSOR_X(names);
  DEFINE_ACCESSOR_X(expression);
};

} // namespace l3::ast
