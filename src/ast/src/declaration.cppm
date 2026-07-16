export module l3.ast:declaration;

import :expression;
import :name_list;
import utils;
import std;
import l3.location;

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
    static constexpr auto format(const auto &op, std::format_context &ctx) {
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

export namespace l3::ast {

class Declaration {
  NameList names;
  std::optional<Expression> expression;
  Mutability mutability = Mutability::Immutable;

  DEFINE_LOCATION_FIELD()

public:
  Declaration(location::Location location = {});
  Declaration(
      NameList &&names,
      std::optional<Expression> &&expression,
      Mutability mutability,
      location::Location location = {}
  );

  DEFINE_ACCESSOR_X(names);
  DEFINE_ACCESSOR_X(expression);
  DEFINE_VALUE_ACCESSOR_X(mutability);

  [[nodiscard]] bool is_const() const {
    return mutability == Mutability::Immutable;
  }
  [[nodiscard]] bool is_mutable() const {
    return mutability == Mutability::Mutable;
  }
};

} // namespace l3::ast
