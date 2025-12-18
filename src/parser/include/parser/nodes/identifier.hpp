#pragma once

#include <deque>
#include <string>

namespace l3::ast {

class Identifier {
  std::string id;

public:
  Identifier() = default;
  Identifier(std::string &&id) : id(std::move(id)) {}

  [[nodiscard]] const std::string &name() const { return id; }

  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

class Variable {
  Identifier id;

public:
  Variable() = default;
  Variable(Identifier &&id) : id(std::move(id)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const;
};

class NameList : public std::deque<Identifier> {
public:
  NameList() = default;
  NameList(Identifier &&ident) : std::deque<Identifier>({std::move(ident)}) {};
  NameList &&with_name(Identifier &&ident);
};

} // namespace l3::ast
