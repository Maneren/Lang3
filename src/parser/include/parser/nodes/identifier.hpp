#pragma once

#include "parser/nodes/detail.hpp"
#include <string>
#include <vector>

namespace l3::ast {

class Identifier {
  std::string id;

public:
  Identifier() = default;
  Identifier(std::string &&id) : id(std::move(id)) {}

  [[nodiscard]] const std::string &name() const { return id; }

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::format_indented_line(out, depth, "Identifier '{}'", id);
  }
};

class Variable {
  Identifier id;

public:
  Variable() = default;
  Variable(Identifier &&id) : id(std::move(id)) {}

  void print(std::output_iterator<char> auto &out, size_t depth) const {
    detail::format_indented_line(out, depth, "Variable '{}'", id.name());
  }
};
using NameList = std::vector<Identifier>;

} // namespace l3::ast
