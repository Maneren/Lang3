#pragma once

#include "statement.hpp"
#include "utils/accessor.h"
#include <cstddef>
#include <deque>
#include <iterator>
#include <optional>

namespace l3::ast {

class Block {
  std::deque<Statement> statements;
  std::optional<LastStatement> last_statement;

public:
  Block() = default;
  Block(Statement &&statement);
  Block(LastStatement &&lastStatement);

  Block &with_statement(Statement &&statement);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_ACCESSOR(statements, std::deque<Statement>, statements)
  DEFINE_ACCESSOR(last_statement, std::optional<LastStatement>, last_statement)
};

class Program : public Block {
public:
  Program() = default;
  Program(Block &&block) : Block(std::move(block)) {}
};

} // namespace l3::ast
