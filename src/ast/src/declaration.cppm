export module l3.ast:declaration;

import :expression;
import :mutability;
import :name_list;
import l3.location;

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
