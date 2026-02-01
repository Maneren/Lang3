module;

#include <format>
#include <memory>
#include <optional>
#include <utils/accessor.h>

export module l3.ast:range_for_loop;

import :identifier;
import :mutability;

export namespace l3::ast {

class Block;
class Expression;

enum class RangeOperator : std::uint8_t { Inclusive, Exclusive };

class RangeForLoop {
  Identifier variable;
  std::unique_ptr<Expression> start;
  std::unique_ptr<Expression> end;
  std::optional<std::unique_ptr<Expression>> step;
  std::unique_ptr<Block> body;
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
  DEFINE_PTR_ACCESSOR_X(start);
  DEFINE_PTR_ACCESSOR_X(end);
  DEFINE_ACCESSOR_X(step);
  DEFINE_PTR_ACCESSOR_X(body);
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

template <>
struct std::formatter<l3::ast::RangeOperator>
    : utils::static_formatter<l3::ast::RangeOperator> {
  static constexpr auto
  format(l3::ast::RangeOperator range_type, std::format_context &ctx) {
    switch (range_type) {
    case l3::ast::RangeOperator::Inclusive:
      return std::format_to(ctx.out(), "Inclusive");
    case l3::ast::RangeOperator::Exclusive:
      return std::format_to(ctx.out(), "Exclusive");
    }
  }
};
