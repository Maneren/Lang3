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
  Block(Statement &&statement) {
    statements.emplace_front(std::move(statement));
  }
  Block(LastStatement &&lastStatement)
      : lastStatement(std::move(lastStatement)) {}

  Block &&with_statement(Statement &&statement);

  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

} // namespace l3::ast
