#pragma once

#include "expression.hpp"
#include "identifier.hpp"
#include <memory>
#include <variant>

namespace l3::ast {

class Block;
struct FunctionBody {
  NameList parameters;
  std::shared_ptr<Block> block;
};

struct NamedFunction {
  Identifier name;
  FunctionBody body;
};

using AnonymousFunction = FunctionBody;

using Function = std::variant<AnonymousFunction, NamedFunction>;

using Arguments = ExpressionList;

} // namespace l3::ast
