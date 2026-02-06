export module l3.ast:mutability;

import utils;

export {
  namespace l3::ast {

  enum class Mutability : std::uint8_t {
    Immutable,
    Mutable,
  };

  } // namespace l3::ast

  template <>
  struct std::formatter<l3::ast::Mutability>
      : utils::static_formatter<l3::ast::Mutability> {
    static constexpr auto format(auto op, std::format_context &ctx) {
      using namespace l3::ast;
      switch (op) {
      case Mutability::Immutable:
        return std::format_to(ctx.out(), "Immutable");
      case Mutability::Mutable:
        return std::format_to(ctx.out(), "Mutable");
      }
    }
  };
}
