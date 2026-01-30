module;

#include <memory>
#include <utils/accessor.h>
#include <utils/match.h>

export module l3.ast:function;

import :identifier;
import :name_list;
import :printing;

export namespace l3::ast {

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
  FunctionBody(NameList &&parameters, std::shared_ptr<Block> &&block)
      : parameters(std::move(parameters)), block(std::move(block)) {};

  ~FunctionBody();

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_ACCESSOR_X(parameters);
  DEFINE_PTR_ACCESSOR(block, Block, block)
  [[nodiscard]] std::shared_ptr<Block> get_block_ptr() const { return block; }
  std::shared_ptr<Block> get_block_ptr_mut() { return block; }
};

class NamedFunction {
  Identifier name;
  FunctionBody body;

public:
  NamedFunction() = default;
  NamedFunction(Identifier &&name, FunctionBody &&body)
      : name(std::move(name)), body(std::move(body)) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "NamedFunction");
    name.print(out, depth + 1);
    body.print(out, depth + 1);
  };

  DEFINE_ACCESSOR_X(name);
  DEFINE_ACCESSOR_X(body);
};

class AnonymousFunction {
  FunctionBody body;

public:
  AnonymousFunction() = default;
  AnonymousFunction(FunctionBody &&body) : body(std::move(body)) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "AnonymousFunction");
    body.print(out, depth + 1);
  }

  DEFINE_ACCESSOR_X(body);
};

} // namespace l3::ast
