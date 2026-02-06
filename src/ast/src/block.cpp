module;

#include <utility>

module l3.ast;

namespace l3::ast {

Block::Block() = default;
Block::Block(Statement &&statement) {
  statements.emplace_front(std::move(statement));
}
Block::Block(LastStatement &&last_statement)
    : last_statement(
          std::make_unique<LastStatement>(std::move(last_statement))
      ) {}

Block &Block::with_statement(Statement &&statement) {
  statements.push_front(std::move(statement));
  return *this;
}

Program::Program() = default;
Program::Program(Block &&block) : Block(std::move(block)) {}

} // namespace l3::ast
