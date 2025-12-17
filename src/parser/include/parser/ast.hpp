#pragma once

#include "nodes/block.hpp"
#include "nodes/printing.hpp"

#include <memory>
#include <optional>

namespace l3::ast {

// enum class Separator : uint8_t { Comma, Semicolon };

struct IfClause;

// using ElseClause =
//     std::variant<std::shared_ptr<Block>, std::shared_ptr<IfClause>>;

struct IfClause {
  Expression condition;
  std::shared_ptr<Block> block;
  std::optional<std::shared_ptr<Block>> elseBlock;
};

using Program = Block;

} // namespace l3::ast
