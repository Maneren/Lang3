module;

#include <utility>

module l3.ast;

import :identifier;

namespace l3::ast {

NameList::NameList() = default;
NameList::NameList(Identifier &&ident) { emplace_front(std::move(ident)); }
NameList &NameList::with_name(Identifier &&ident) {
  emplace_front(std::move(ident));
  return *this;
}

} // namespace l3::ast
