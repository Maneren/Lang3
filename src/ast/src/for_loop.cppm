export module l3.ast:for_loop;

import :block;
import :identifier;
import :expression;
import :mutability;
import l3.location;

export namespace l3::ast {

class ForLoop {
  Identifier variable;
  Expression collection;
  Block body;
  Mutability mutability = Mutability::Immutable;

  DEFINE_LOCATION_FIELD()

public:
  ForLoop(location::Location location = {});
  ForLoop(
      Identifier &&variable,
      Expression &&collection,
      Block &&body,
      Mutability mutability,
      location::Location location = {}
  );

  ForLoop(const ForLoop &) = delete;
  ForLoop(ForLoop &&) noexcept;
  ForLoop &operator=(const ForLoop &) = delete;
  ForLoop &operator=(ForLoop &&) noexcept;
  ~ForLoop();

  DEFINE_ACCESSOR_X(variable);
  DEFINE_ACCESSOR_X(collection);
  DEFINE_ACCESSOR_X(body);
  DEFINE_VALUE_ACCESSOR_X(mutability);

  [[nodiscard]] bool is_const() const {
    return mutability == Mutability::Immutable;
  }
  [[nodiscard]] bool is_mutable() const {
    return mutability == Mutability::Mutable;
  }
};

} // namespace l3::ast
