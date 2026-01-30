module;

#include <deque>
#include <string>
#include <utility>

export module l3.ast:name_list;

import :identifier;

export namespace l3::ast {

class NameList : public std::deque<Identifier> {
public:
  NameList() = default;
  NameList(Identifier &&ident) { emplace_front(std::move(ident)); }
  NameList &with_name(Identifier &&ident) {
    emplace_front(std::move(ident));
    return *this;
  }

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    for (const auto &name : *this) {
      name.print(out, depth);
    }
  }
};

} // namespace l3::ast
