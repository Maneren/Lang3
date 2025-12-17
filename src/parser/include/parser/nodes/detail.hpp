#pragma once

#include <format>
#include <iterator>

namespace l3::ast::detail {

void indent(std::output_iterator<char> auto &out, size_t depth) {
  for (size_t i = 0; i < depth; ++i) {
    std::format_to(out, " ");
  }
}

} // namespace l3::ast::detail
