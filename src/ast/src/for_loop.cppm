module;

#include <memory>
#include <utils/accessor.h>

export module l3.ast:for_loop;

import :identifier;
import :mutability;

export namespace l3::ast {

class Block;
class Expression;

class ForLoop {
  Identifier variable;
  std::unique_ptr<Expression> collection;
  std::unique_ptr<Block> body;
  Mutability mutability = Mutability::Immutable;

public:
  ForLoop();
  ForLoop(
      Identifier &&variable,
      Expression &&collection,
      Block &&body,
      Mutability mutability
  );

  ForLoop(const ForLoop &) = delete;
  ForLoop(ForLoop &&) noexcept;
  ForLoop &operator=(const ForLoop &) = delete;
  ForLoop &operator=(ForLoop &&) noexcept;
  ~ForLoop();

  DEFINE_ACCESSOR_X(variable);
  DEFINE_PTR_ACCESSOR_X(collection);
  DEFINE_PTR_ACCESSOR_X(body);
  DEFINE_VALUE_ACCESSOR_X(mutability);

  [[nodiscard]] bool is_const() const {
    return mutability == Mutability::Immutable;
  }
  [[nodiscard]] bool is_mutable() const {
    return mutability == Mutability::Mutable;
  }
};

} // namespace l3::ast
