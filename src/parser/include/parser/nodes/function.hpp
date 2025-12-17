#pragma once

#include "identifier.hpp"
#include <memory>

namespace l3::ast {

class Block;

class FunctionBody {
  NameList parameters;
  std::shared_ptr<Block> block;

public:
  FunctionBody() = default;

  FunctionBody(NameList &&parameters, std::shared_ptr<Block> &&block)
      : parameters(std::move(parameters)), block(std::move(block)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

class NamedFunction {
  Identifier name;
  FunctionBody body;

public:
  NamedFunction() = default;
  NamedFunction(Identifier &&name, FunctionBody &&body)
      : name(std::move(name)), body(std::move(body)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

class AnonymousFunction {
  FunctionBody body;

public:
  AnonymousFunction() = default;
  AnonymousFunction(FunctionBody &&body) : body(std::move(body)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

} // namespace l3::ast
