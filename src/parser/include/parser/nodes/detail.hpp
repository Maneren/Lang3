#pragma once

#include <format>
#include <iterator>

namespace l3::ast::detail {

constexpr void indent(std::output_iterator<char> auto &out, size_t depth) {
  for (size_t i = 0; i < depth; ++i) {
    std::format_to(out, " ");
  }
}

template <typename... Args>
constexpr void format_indented(
    std::output_iterator<char> auto &out,
    size_t depth,
    std::format_string<Args...> fmt,
    Args &&...args
) {
  detail::indent(out, depth);
  std::format_to(out, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
constexpr void format_indented_line(
    std::output_iterator<char> auto &out,
    size_t depth,
    std::format_string<Args...> fmt,
    Args &&...args
) {
  detail::indent(out, depth);
  std::format_to(out, fmt, std::forward<Args>(args)...);
  std::format_to(out, "\n");
}

} // namespace l3::ast::detail
