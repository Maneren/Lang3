module;

#include <utility>
#include <utils/accessor.h>

export module l3.ast:function_call;

import :identifier;
import :expression_list;

export namespace l3::ast {

class FunctionCall {
  Identifier name;
  ExpressionList arguments;

public:
  FunctionCall() = default;
  FunctionCall(Identifier &&name, ExpressionList &&args)
      : name(std::move(name)), arguments(std::move(args)) {}

  DEFINE_ACCESSOR_X(name);
  DEFINE_ACCESSOR_X(arguments);
};

} // namespace l3::ast
