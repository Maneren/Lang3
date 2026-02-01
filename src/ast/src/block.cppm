module;

#include <deque>
#include <optional>
#include <utility>
#include <utils/accessor.h>

export module l3.ast:block;

import :statement;
import :last_statement;

export namespace l3::ast {

class Block {
  std::deque<Statement> statements;
  std::optional<LastStatement> last_statement;

public:
  Block() = default;
  Block(Statement &&statement) {
    statements.emplace_front(std::move(statement));
  }
  Block(LastStatement &&lastStatement)
      : last_statement(std::move(lastStatement)) {}

  Block &with_statement(Statement &&statement) {
    statements.push_front(std::move(statement));
    return *this;
  }

  DEFINE_ACCESSOR_X(statements)
  DEFINE_ACCESSOR_X(last_statement)
};

class Program : public Block {
public:
  Program() = default;
  Program(Block &&block) : Block(std::move(block)) {}
};

} // namespace l3::ast
