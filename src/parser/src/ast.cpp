#include "parser/nodes/block.hpp"
#include "parser/nodes/statement.hpp"
#include <utility>

namespace l3::ast {

void Block::add_statement(Statement &&statement) {
  statements.push_front(std::move(statement));
}

} // namespace l3::ast
