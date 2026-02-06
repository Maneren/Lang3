module;

#include <memory>
#include <optional>
#include <utils/accessor.h>

export module l3.ast:range_for_loop;

import :block;
import :expression;
import :identifier;
import :mutability;
export import :range_operator;

export namespace l3::ast {

class Expression;

class RangeForLoop {
  Identifier variable;
  Expression start;
  Expression end;
  std::optional<Expression> step;
  Block body;
  RangeOperator range_type = RangeOperator::Inclusive;
  Mutability mutability = Mutability::Immutable;

public:
  RangeForLoop();
  RangeForLoop(
      Mutability mutability,
      Identifier &&variable,
      Expression &&start,
      RangeOperator range_type,
      Expression &&end,
      std::optional<Expression> &&step,
      Block &&body
  );

  RangeForLoop(const RangeForLoop &) = delete;
  RangeForLoop(RangeForLoop &&) noexcept;
  RangeForLoop &operator=(const RangeForLoop &) = delete;
  RangeForLoop &operator=(RangeForLoop &&) noexcept;
  ~RangeForLoop();

  DEFINE_ACCESSOR_X(variable);
  DEFINE_ACCESSOR_X(start);
  DEFINE_ACCESSOR_X(end);
  DEFINE_ACCESSOR_X(step);
  DEFINE_ACCESSOR_X(body);
  DEFINE_VALUE_ACCESSOR_X(range_type);
  DEFINE_VALUE_ACCESSOR_X(mutability);

  [[nodiscard]] bool is_const() const {
    return mutability == Mutability::Immutable;
  }
  [[nodiscard]] bool is_mutable() const {
    return mutability == Mutability::Mutable;
  }
};

} // namespace l3::ast
