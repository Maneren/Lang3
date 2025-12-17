#pragma once

#include "parser/nodes/detail.hpp"
#include <string>
#include <vector>

namespace l3::ast {

class Identifier {
protected:
  std::string id;

public:
  Identifier() = default;
  Identifier(std::string &&id) : id(std::move(id)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::indent(out, depth);
    std::format_to(out, "Identifier '{}'\n", id);
  }
};

class Variable : public Identifier {
public:
  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::indent(out, depth);
    std::format_to(out, "Variable '{}'\n", id);
  }
};
using NameList = std::vector<Identifier>;

} // namespace l3::ast
