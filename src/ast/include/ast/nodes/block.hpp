#pragma once

#include "statement.hpp"
#include <cstddef>
#include <deque>
#include <iterator>
#include <optional>

namespace l3::ast {

class Block {
  std::deque<Statement> statements;
  std::optional<LastStatement> lastStatement;

public:
  Block() = default;
  Block(Statement &&statement);
  Block(LastStatement &&lastStatement);

  Block &with_statement(Statement &&statement);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] const std::deque<Statement> &get_statements() const {
    return statements;
  }
  [[nodiscard]] std::deque<Statement> &get_statements_mut() {
    return statements;
  }
  [[nodiscard]] const std::optional<LastStatement> &get_last_statement() const {
    return lastStatement;
  }
  [[nodiscard]] std::optional<LastStatement> &get_last_statement_mut() {
    return lastStatement;
  }
};

class Program : public Block {
public:
  Program() = default;
  Program(Block &&block) : Block(std::move(block)) {}
};

} // namespace l3::ast
