module;

#include <utility>
#include <utils/accessor.h>

export module l3.ast:name_assignment;

import :expression;
import :name_list;

export namespace l3::ast {

class NameAssignment {
  NameList names;
  Expression expression;

public:
  NameAssignment() = default;
  NameAssignment(NameList &&names, Expression &&expression)
      : names(std::move(names)), expression(std::move(expression)) {}

  DEFINE_ACCESSOR_X(names);
  DEFINE_ACCESSOR_X(expression);
};

} // namespace l3::ast
