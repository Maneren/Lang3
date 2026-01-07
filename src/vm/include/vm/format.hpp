#pragma once

#include "utils/format.h"
#include "vm/function.hpp"

template <>
struct std::formatter<l3::vm::Nil> : utils::static_formatter<l3::vm::Nil> {
  static constexpr auto
  format(const auto & /*unused*/, std::format_context &ctx) {
    return std::format_to(ctx.out(), "nil");
  }
};

template <>
struct std::formatter<l3::vm::Primitive>
    : utils::static_formatter<l3::vm::Primitive> {
  static constexpr auto
  format(const auto &primitive, std::format_context &ctx) {
    return match::match(
        primitive,
        [&ctx](const bool &value) {
          return std::format_to(ctx.out(), "{}", value);
        },
        [&ctx](const long long &value) {
          return std::format_to(ctx.out(), "{}", value);
        },
        [&ctx](const double &value) {
          return std::format_to(ctx.out(), "{}", value);
        },
        [&ctx](const std::shared_ptr<std::string> &value) {
          return std::format_to(ctx.out(), "\"{}\"", *value);
        }
    );
  }
};

template <>
struct std::formatter<l3::vm::L3Function>
    : utils::static_formatter<l3::vm::L3Function> {
  static constexpr auto format(const auto &value, std::format_context &ctx) {
    return std::format_to(ctx.out(), "function <{}>", value.get_name());
  }
};

template <>
struct std::formatter<l3::vm::BuiltinFunction>
    : utils::static_formatter<l3::vm::BuiltinFunction> {
  static constexpr auto format(const auto &value, std::format_context &ctx) {
    return std::format_to(ctx.out(), "function <{}>", value.get_name());
  }
};

template <>
struct std::formatter<l3::vm::Function>
    : utils::static_formatter<l3::vm::Function> {
  static constexpr auto format(const auto &value, std::format_context &ctx) {
    return value.visit(
        [&ctx](const l3::vm::L3Function &value) {
          return std::format_to(ctx.out(), "{}", value);
        },
        [&ctx](const l3::vm::BuiltinFunction &value) {
          return std::format_to(ctx.out(), "{}", value);
        }
    );
  }
};

template <>
struct std::formatter<l3::vm::Value> : utils::static_formatter<l3::vm::Value> {
  static constexpr auto format(const auto &value, std::format_context &ctx) {
    return value.visit(
        [&ctx](const l3::vm::Nil &value) {
          return std::format_to(ctx.out(), "{}", value);
        },
        [&ctx](const l3::vm::Primitive &primitive) {
          return std::format_to(ctx.out(), "{}", primitive);
        },
        [&ctx](const std::shared_ptr<l3::vm::Function> &function) {
          return std::format_to(ctx.out(), "{}", *function);
        }
    );
  }
};
