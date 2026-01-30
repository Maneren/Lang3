module;

#include <format>
#include <string>
#include <utils/accessor.h>

export module l3.ast:identifier;

import :printing;

export namespace l3::ast {

class Identifier {
  std::string name;

public:
  Identifier() = default;
  Identifier(std::string &&name) : name(std::move(name)) {};
  Identifier(std::string_view name) : name(name) {}

  Identifier(const Identifier &other) = default;
  Identifier &operator=(const Identifier &other) = default;
  Identifier(Identifier &&other) noexcept = default;
  Identifier &operator=(Identifier &&other) noexcept = default;
  ~Identifier() = default;

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Identifier '{}'", name);
  }

  DEFINE_ACCESSOR_X(name);

  std::compare_three_way operator<=>(const Identifier &) const = default;
};

} // namespace l3::ast

export template <> struct std::formatter<l3::ast::Identifier> {
  static constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
  static constexpr auto
  format(l3::ast::Identifier const &id, std::format_context &ctx) {
    return std::format_to(ctx.out(), "{}", id.get_name());
  }
};
