export module l3.ast:block;

import utils;

export namespace l3::ast {

class Statement;
class LastStatement;

class Block {
  std::deque<Statement> statements;
  std::unique_ptr<LastStatement> last_statement;

public:
  Block();
  Block(Statement &&statement);
  Block(LastStatement &&lastStatement);

  Block &with_statement(Statement &&statement);

  DEFINE_ACCESSOR_X(statements)
  [[nodiscard]] utils::optional_cref<LastStatement> get_last_statement() const {
    if (last_statement) {
      return std::cref(*last_statement);
    }

    return std::nullopt;
  }
};

class Program : public Block {
public:
  Program();
  Program(Block &&block);
};

} // namespace l3::ast
