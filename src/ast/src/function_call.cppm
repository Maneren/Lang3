export module l3.ast:function_call;

import l3.location;

import :expression_list;
import :identifier;

export namespace l3::ast {

class FunctionCall {
  Identifier name;
  ExpressionList arguments;

  DEFINE_LOCATION_FIELD()

public:
  FunctionCall(location::Location location = {});
  FunctionCall(
      Identifier &&name, ExpressionList &&args, location::Location location = {}
  );

  DEFINE_ACCESSOR_X(name);
  DEFINE_ACCESSOR_X(arguments);
};

} // namespace l3::ast
