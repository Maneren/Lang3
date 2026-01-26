#pragma once

#include "utils/accessor.h"
#include <compare>
#include <cstddef>
#include <deque>
#include <iterator>
#include <string>
#include <string_view>

namespace l3::ast {

class Identifier {
  std::string id;

public:
  Identifier() = default;
  Identifier(std::string &&id);
  Identifier(std::string_view id);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_ACCESSOR(name, std::string, id);

  std::compare_three_way operator<=>(const Identifier &) const = default;
};

class Variable {
  Identifier id;

public:
  Variable() = default;
  Variable(Identifier &&id);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  DEFINE_ACCESSOR(identifier, Identifier, id);
};

class NameList : public std::deque<Identifier> {
public:
  NameList() = default;
  NameList(Identifier &&ident);
  NameList &with_name(Identifier &&ident);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;
};

} // namespace l3::ast
