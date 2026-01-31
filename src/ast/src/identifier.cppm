module;

#include <format>
#include <string>
#include <utils/accessor.h>

export module l3.ast:identifier;

import utils;
import :printing;

export namespace l3::ast {

class Identifier {
  std::string name;

public:
  Identifier() = default;
  Identifier(std::string &&name) : name(std::move(name)) {};
  Identifier(std::string_view name) : name(name) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Identifier '{}'", name);
  }

  DEFINE_ACCESSOR_X(name);

  [[nodiscard]] constexpr std::compare_three_way
  operator<=>(const Identifier &) const = default;
};

} // namespace l3::ast

export template <>
struct std::formatter<l3::ast::Identifier>
    : utils::static_formatter<l3::ast::Identifier> {
  static constexpr auto
  format(l3::ast::Identifier const &id, std::format_context &ctx) {
    return std::format_to(ctx.out(), "{}", id.get_name());
  }
};
