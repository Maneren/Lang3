#pragma once

#include "nodes/statement.hpp"

#include <format>
#include <iterator>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace l3::ast {

// enum class Separator : uint8_t { Comma, Semicolon };

class Block;

struct IfClause;

// using ElseClause =
//     std::variant<std::shared_ptr<Block>, std::shared_ptr<IfClause>>;

struct IfClause {
  Expression condition;
  std::shared_ptr<Block> block;
  std::optional<std::shared_ptr<Block>> elseBlock;
};

class Block {
  std::vector<Statement> statements;
  std::optional<LastStatement> lastStatement;

public:
  Block() = default;
  Block(std::vector<Statement> &&statements)
      : statements(std::move(statements)) {}
  Block(std::vector<Statement> &&statements, LastStatement &&lastStatement)
      : statements(std::move(statements)),
        lastStatement(std::move(lastStatement)) {}

  void add_statement(Statement &&statement);

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::indent(out, depth);
    std::format_to(out, "Block\n");
    for (const Statement &statement : statements) {
      statement.print(out, depth + 1);
    }
    if (lastStatement) {
      lastStatement->print(out, depth + 1);
    }
  }
};

using Program = Block;

} // namespace l3::ast
