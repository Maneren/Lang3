module;

#include <utils/accessor.h>

export module l3.ast:array;

import :expression_list;

export namespace l3::ast {

class Array {
  ExpressionList elements;

public:
  Array();
  Array(ExpressionList &&elements);

  DEFINE_ACCESSOR_X(elements);
};

} // namespace l3::ast
