export module l3.ast:array;

import std;

import l3.location;

import :expression_list;

export namespace l3::ast {

class Array {
  ExpressionList elements;

  DEFINE_LOCATION_FIELD()

public:
  Array(location::Location location = {});
  Array(ExpressionList &&elements, location::Location location = {});

  DEFINE_ACCESSOR_X(elements);
};

} // namespace l3::ast
