#pragma once

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

  [[nodiscard]] const std::string &get_name() const;
  [[nodiscard]] std::string &get_name_mut() { return id; }

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  std::compare_three_way operator<=>(const Identifier &) const = default;
};

class Variable {
  Identifier id;

public:
  Variable() = default;
  Variable(Identifier &&id);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;

  [[nodiscard]] const Identifier &get_identifier() const;
  Identifier &get_identifier_mut() { return id; }
};

class NameList : public std::deque<Identifier> {
public:
  NameList() = default;
  NameList(Identifier &&ident);
  NameList &with_name(Identifier &&ident);

  void print(std::output_iterator<char> auto &out, std::size_t depth = 0) const;
};

} // namespace l3::ast
