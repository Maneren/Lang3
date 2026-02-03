module;

#include <optional>
#include <utils/accessor.h>

export module l3.ast:declaration;

import :expression;
import :mutability;
import :name_list;

export namespace l3::ast {

class Declaration {
  NameList names;
  std::optional<Expression> expression;
  Mutability mutability = Mutability::Immutable;

public:
  Declaration();
  Declaration(
      NameList &&names,
      std::optional<Expression> &&expression,
      Mutability mutability
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
