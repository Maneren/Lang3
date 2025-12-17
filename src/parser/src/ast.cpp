#include "parser/nodes/block.hpp"
#include "parser/nodes/statement.hpp"
#include <utility>

namespace l3::ast {

Block &&Block::with_statement(Statement &&statement) {
  statements.push_front(std::move(statement));
  return std::move(*this);
}

} // namespace l3::ast
