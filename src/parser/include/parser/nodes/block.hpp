#pragma once

#include "statement.hpp"

#include <deque>
#include <iterator>
#include <optional>

namespace l3::ast {

class Block {
  std::deque<Statement> statements;
  std::optional<LastStatement> lastStatement;

public:
  Block() = default;
  Block(Statement &&statement) : statements({std::move(statement)}) {}
  Block(LastStatement &&lastStatement)
      : lastStatement(std::move(lastStatement)) {}

  void add_statement(Statement &&statement);

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::format_indented_line(out, depth, "Block");
    for (const Statement &statement : statements) {
      statement.print(out, depth + 1);
    }
    if (lastStatement) {
      detail::format_indented_line(out, depth + 1, "LastStatement");
      lastStatement->print(out, depth + 2);
    }
  }
};

} // namespace l3::ast
