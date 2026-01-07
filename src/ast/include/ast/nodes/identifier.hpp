#pragma once

#include <deque>
#include <string>

namespace l3::ast {

class Identifier {
  std::string id;

public:
  Identifier() = default;
  Identifier(std::string &&id) : id(std::move(id)) {}
  Identifier(std::string_view id) : id(id) {}

  [[nodiscard]] const std::string &name() const { return id; }

  void print(std::output_iterator<char> auto &out, size_t depth = 0) const;

  std::compare_three_way operator<=>(const Identifier &) const = default;
};

class Variable {
  Identifier id;

public:
  Variable() = default;
  Variable(Identifier &&id) : id(std::move(id)) {}

  void print(std::output_iterator<char> auto &out, size_t depth = 0) const;

  [[nodiscard]] const Identifier &get_identifier() const { return id; }
};

class NameList : public std::deque<Identifier> {
public:
  NameList() = default;
  NameList(Identifier &&ident);
  NameList &with_name(Identifier &&ident);
};

} // namespace l3::ast
