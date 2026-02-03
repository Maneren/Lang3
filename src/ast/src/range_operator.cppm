module;

#include <cstdint>
#include <format>

export module l3.ast:range_operator;

import utils;

export namespace l3::ast {

enum class RangeOperator : std::uint8_t { Inclusive, Exclusive };

}

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
