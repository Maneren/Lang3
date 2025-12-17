#pragma once

#include "identifier.hpp"
#include <memory>
#include <variant>

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

using AnonymousFunction = FunctionBody;

class Function {
  std::variant<AnonymousFunction, NamedFunction> inner;

public:
  Function() = default;

  Function(AnonymousFunction &&function) : inner(std::move(function)) {}
  Function(NamedFunction &&function) : inner(std::move(function)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    inner.visit([&out, depth](const auto &node) -> void {
      node.print(out, depth);
    });
  }
};

} // namespace l3::ast
