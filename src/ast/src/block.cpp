module;

#include <utility>

module l3.ast;

namespace l3::ast {

Block::Block() = default;
Block::Block(Statement &&statement) {
  statements.emplace_front(std::move(statement));
}
Block::Block(LastStatement &&lastStatement)
    : last_statement(std::move(lastStatement)) {}

Block &Block::with_statement(Statement &&statement) {
  statements.push_front(std::move(statement));
  return *this;
}

Program::Program() = default;
Program::Program(Block &&block) : Block(std::move(block)) {}

} // namespace l3::ast
