module;

#include <deque>
#include <optional>
#include <utils/accessor.h>

export module l3.ast:block;

import :statement;
import :last_statement;

export namespace l3::ast {

class Block {
  std::deque<Statement> statements;
  std::optional<LastStatement> last_statement;

public:
  Block();
  Block(Statement &&statement);
  Block(LastStatement &&lastStatement);

  Block &with_statement(Statement &&statement);

  DEFINE_ACCESSOR_X(statements)
  DEFINE_ACCESSOR_X(last_statement)
};

class Program : public Block {
public:
  Program();
  Program(Block &&block);
};

} // namespace l3::ast
