#pragma once

#include "ast/nodes/expression_list.hpp"
#include "identifier.hpp"
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>

namespace l3::ast {

using Arguments = ExpressionList;

class FunctionCall {
  Variable name;
  Arguments args;

public:
  FunctionCall() = default;
  FunctionCall(Variable &&name, Arguments &&args);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] const Variable &get_name() const { return name; }
  [[nodiscard]] const Arguments &get_arguments() const { return args; }
};

class Block;

class FunctionBody {
  NameList parameters;
  std::shared_ptr<Block> block;

public:
  FunctionBody() = default;
  FunctionBody(const FunctionBody &) = default;
  FunctionBody(FunctionBody &&) noexcept;
  FunctionBody &operator=(const FunctionBody &) = default;
  FunctionBody &operator=(FunctionBody &&) noexcept;
  FunctionBody(NameList &&parameters, Block &&block);
  FunctionBody(NameList &&parameters, std::shared_ptr<Block> &&block);

  ~FunctionBody();

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] const NameList &get_parameters() const { return parameters; }
  [[nodiscard]] const Block &get_block() const { return *block; }
  [[nodiscard]] std::shared_ptr<Block> get_block_ptr() const { return block; }
};

class NamedFunction {
  Identifier name;
  FunctionBody body;

public:
  NamedFunction() = default;
  NamedFunction(Identifier &&name, FunctionBody &&body);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] const Identifier &get_name() const { return name; }
  [[nodiscard]] const FunctionBody &get_body() const { return body; }
};

class AnonymousFunction {
  FunctionBody body;

public:
  AnonymousFunction() = default;
  AnonymousFunction(FunctionBody &&body) : body(std::move(body)) {}

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] const FunctionBody &get_body() const { return body; }
};

} // namespace l3::ast
