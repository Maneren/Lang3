#include "parser/ast.hpp"

namespace l3::ast {

void Block::add_statement(Statement &&statement) {
  statements.push_back(std::move(statement));
}

} // namespace l3::ast
