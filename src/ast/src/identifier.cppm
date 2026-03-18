export module l3.ast:identifier;

import utils;
import std;
import l3.location;

export namespace l3::ast {

class Identifier {
  std::string name;

  DEFINE_LOCATION_FIELD()

public:
  Identifier();
  Identifier(std::string &&name, location::Location location = {});
  Identifier(std::string_view name, location::Location location = {});
  Identifier(const char *name, location::Location location = {});

  DEFINE_ACCESSOR_X(name);

  [[nodiscard]]
  constexpr bool operator==(const Identifier &other) const {
    return name == other.name;
  }
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
