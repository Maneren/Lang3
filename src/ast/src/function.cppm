module;

#include <memory>
#include <utility>
#include <utils/accessor.h>

export module l3.ast:function;

import utils;
import :block;
import :identifier;
import :name_list;

export namespace l3::ast {

class Block;

class FunctionBody {
  NameList parameters;
  Block block;

public:
  FunctionBody();
  FunctionBody(NameList &&parameters, Block &&block);

  [[nodiscard]] std::span<const Identifier> get_parameters() const {
    return parameters;
  }
  DEFINE_ACCESSOR_X(block)
};

class NamedFunction {
  Identifier name;
  FunctionBody body;

public:
  NamedFunction();
  NamedFunction(Identifier &&name, FunctionBody &&body);

  DEFINE_ACCESSOR_X(name);
  DEFINE_ACCESSOR_X(body);
};

class AnonymousFunction {
  FunctionBody body;

public:
  AnonymousFunction();
  AnonymousFunction(FunctionBody &&body);

  DEFINE_ACCESSOR_X(body);
};

} // namespace l3::ast
