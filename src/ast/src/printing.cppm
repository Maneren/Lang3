module;

#include <format>
#include <iterator>

export module l3.ast:printing;

constexpr void indent(std::output_iterator<char> auto &out, std::size_t depth) {
  for (std::size_t i = 0; i < depth; ++i) {
    std::format_to(out, "▏ ");
  }
}

template <typename... Args>
constexpr void format_indented(
    std::output_iterator<char> auto &out,
    std::size_t depth,
    std::format_string<Args...> fmt,
    Args &&...args
) {
  indent(out, depth);
  std::format_to(out, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
constexpr void format_indented_line(
    std::output_iterator<char> auto &out,
    std::size_t depth,
    std::format_string<Args...> fmt,
    Args &&...args
) {
  indent(out, depth);
  std::format_to(out, fmt, std::forward<Args>(args)...);
  std::format_to(out, "\n");
}
