module;

#include <deque>
#include <format>
#include <string>
#include <utils/accessor.h>

export module l3.ast:identifier;

import :printing;

export namespace l3::ast {

class Identifier {
  std::string id;

public:
  Identifier() = default;
  Identifier(std::string &&id) : id(std::move(id)) {};
  Identifier(std::string_view id) : id(id) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Identifier '{}'", id);
  }

  DEFINE_ACCESSOR(name, std::string, id);

  std::compare_three_way operator<=>(const Identifier &) const = default;
};

class NameList : public std::deque<Identifier> {
public:
  NameList() = default;
  NameList(Identifier &&ident) { emplace_front(std::move(ident)); }
  NameList &with_name(Identifier &&ident) {
    emplace_front(std::move(ident));
    return *this;
  }

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    for (const auto &name : *this) {
      name.print(out, depth);
    }
  }
};

class Variable {
  Identifier id;

public:
  Variable() = default;
  Variable(Identifier &&id) : id(std::move(id)) {}

  void
  print(std::output_iterator<char> auto &out, std::size_t depth = 0) const {
    format_indented_line(out, depth, "Variable '{}'", id.get_name());
  }

  DEFINE_ACCESSOR(identifier, Identifier, id);
};

} // namespace l3::ast

export template <> struct std::formatter<l3::ast::Identifier> {
  static constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
  static constexpr auto
  format(l3::ast::Identifier const &id, std::format_context &ctx) {
    return std::format_to(ctx.out(), "{}", id.get_name());
  }
};
