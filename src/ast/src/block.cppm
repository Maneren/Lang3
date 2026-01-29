module;

#include <deque>
#include <optional>
#include <utility>
#include <utils/accessor.h>

export module l3.ast:block;

import :printing;
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

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Block");
    for (const Statement &statement : statements) {
      statement.print(out, depth + 1);
    }
    if (last_statement) {
      format_indented_line(out, depth + 0, "LastStatement");
      last_statement->print(out, depth + 1);
    }
  }

  DEFINE_ACCESSOR(statements, std::deque<Statement>, statements)
  DEFINE_ACCESSOR(last_statement, std::optional<LastStatement>, last_statement)
};

class Program : public Block {
public:
  Program() = default;
  Program(Block &&block) : Block(std::move(block)) {}
};

} // namespace l3::ast
