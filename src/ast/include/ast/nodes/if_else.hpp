#pragma once

#include "expression.hpp"
#include <memory>
#include <optional>

namespace l3::ast {

class Block;

// using ElseClause =
//     std::variant<std::unique_ptr<Block>, std::unique_ptr<IfClause>>;

class IfClause {
  Expression condition;
  std::unique_ptr<Block> block;
  std::optional<std::unique_ptr<Block>> elseBlock;

public:
  IfClause() = default;
  IfClause(const IfClause &) = delete;
  IfClause(IfClause &&) noexcept;
  IfClause &operator=(const IfClause &) = delete;
  IfClause &operator=(IfClause &&) noexcept;
  ~IfClause();

  IfClause(Expression &&condition, Block &&block);
  IfClause(Expression &&condition, Block &&block, Block &&elseBlock);

  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

} // namespace l3::ast
