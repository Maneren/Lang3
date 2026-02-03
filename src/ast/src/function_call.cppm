module;

#include <utils/accessor.h>

export module l3.ast:function_call;

import :identifier;
import :expression_list;

export namespace l3::ast {

class FunctionCall {
  Identifier name;
  ExpressionList arguments;

public:
  FunctionCall();
  FunctionCall(Identifier &&name, ExpressionList &&args);

  DEFINE_ACCESSOR_X(name);
  DEFINE_ACCESSOR_X(arguments);
};

} // namespace l3::ast
