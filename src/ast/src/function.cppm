export module l3.ast:function;

import utils;
import :block;
import :identifier;
import :name_list;
import l3.location;

export namespace l3::ast {

class Block;

class FunctionBody {
  NameList parameters;
  Block block;

  DEFINE_LOCATION_FIELD()

public:
  FunctionBody(location::Location location = {});
  FunctionBody(
      NameList &&parameters, Block &&block, location::Location location = {}
  );

  [[nodiscard]] std::span<const Identifier> get_parameters() const {
    return parameters;
  }
  DEFINE_ACCESSOR_X(block)
};

class NamedFunction {
  Identifier name;
  FunctionBody body;

  DEFINE_LOCATION_FIELD()

public:
  NamedFunction(location::Location location = {});
  NamedFunction(
      Identifier &&name, FunctionBody &&body, location::Location location = {}
  );

  DEFINE_ACCESSOR_X(name);
  DEFINE_ACCESSOR_X(body);
};

class AnonymousFunction {
  FunctionBody body;

  DEFINE_LOCATION_FIELD()

public:
  AnonymousFunction(location::Location location = {});
  AnonymousFunction(FunctionBody &&body, location::Location location = {});

  DEFINE_ACCESSOR_X(body);
};

} // namespace l3::ast
