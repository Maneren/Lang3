module;

#include <vector>

export module l3.ast:name_list;

export namespace l3::ast {

class Identifier;

class NameList : public std::vector<Identifier> {
public:
  NameList();
  NameList(Identifier &&ident);
  NameList &with_name(Identifier &&ident);
};

} // namespace l3::ast
