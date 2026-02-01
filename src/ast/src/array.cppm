module;

#include <utility>
#include <utils/accessor.h>

export module l3.ast:array;

import :expression_list;

export namespace l3::ast {

class Array {
  ExpressionList elements;

public:
  Array() = default;
  Array(ExpressionList &&elements) : elements(std::move(elements)) {}

  DEFINE_ACCESSOR_X(elements);
};

} // namespace l3::ast
