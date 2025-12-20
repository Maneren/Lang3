#pragma once

#include "identifier.hpp"
#include <memory>

namespace l3::ast {

class Expression;

class ExpressionList : public std::deque<Expression> {
public:
  ExpressionList() = default;
  ExpressionList(Expression &&expr);
  ExpressionList &with_expression(Expression &&);
};

using Arguments = ExpressionList;

class FunctionCall {
  Variable name;
  Arguments args;

public:
  FunctionCall() = default;
  FunctionCall(Variable &&name, Arguments &&args);

  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

class Block;

class FunctionBody {
  NameList parameters;
  std::unique_ptr<Block> block;

public:
  FunctionBody() = default;
  FunctionBody(const FunctionBody &) = delete;
  FunctionBody(FunctionBody &&) noexcept;
  FunctionBody &operator=(const FunctionBody &) = delete;
  FunctionBody &operator=(FunctionBody &&) noexcept;
  FunctionBody(NameList &&parameters, Block &&block);

  ~FunctionBody();

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
