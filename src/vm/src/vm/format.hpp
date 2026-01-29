#pragma once

#include "vm/function.hpp"
#include "vm/scope.hpp"
#include "vm/storage.hpp"
#include <utils/format.h>

import l3.ast;

template <>
struct std::formatter<l3::vm::Nil> : utils::static_formatter<l3::vm::Nil> {
  static constexpr auto
  format(const auto & /*unused*/, std::format_context &ctx) {
    return std::format_to(ctx.out(), "nil");
  }
};

namespace l3::vm {
struct PrimitivePrettyPrinter {
  const Primitive &primitive; // NOLINT
};
struct ValuePrettyPrinter {
  const Value &value; // NOLINT
};
} // namespace l3::vm

template <>
struct std::formatter<l3::vm::Primitive>
    : utils::static_formatter<l3::vm::Primitive> {
  static constexpr auto
  format(const auto &primitive, std::format_context &ctx) {
    return primitive.visit(
        [&ctx](const bool &value) {
          return std::format_to(ctx.out(), "{}", value);
        },
        [&ctx](const std::int64_t &value) {
          return std::format_to(ctx.out(), "{}", value);
        },
        [&ctx](const double &value) {
          return std::format_to(ctx.out(), "{}", value);
        },
        [&ctx](const l3::vm::Primitive::string_type &value) {
          return std::format_to(ctx.out(), "\"{}\"", value);
        }
    );
  }
};

template <>
struct std::formatter<l3::vm::PrimitivePrettyPrinter>
    : utils::static_formatter<l3::vm::PrimitivePrettyPrinter> {
  static constexpr auto
  format(const auto &primitive_pp, std::format_context &ctx) {
    return primitive_pp.primitive.visit(
        [&ctx](const bool &value) {
          return std::format_to(ctx.out(), "{}", value);
        },
        [&ctx](const std::int64_t &value) {
          return std::format_to(ctx.out(), "{}", value);
        },
        [&ctx](const double &value) {
          return std::format_to(ctx.out(), "{}", value);
        },
        [&ctx](const l3::vm::Primitive::string_type &value) {
          return std::format_to(ctx.out(), "{}", value);
        }
    );
  }
};

template <>
struct std::formatter<l3::vm::L3Function>
    : utils::static_formatter<l3::vm::L3Function> {
  static constexpr auto format(const auto &obj, std::format_context &ctx) {
    return std::format_to(ctx.out(), "function <{}>", obj.get_name());
  }
};

template <>
struct std::formatter<l3::vm::BuiltinFunction>
    : utils::static_formatter<l3::vm::BuiltinFunction> {
  static constexpr auto format(const auto &obj, std::format_context &ctx) {
    return std::format_to(ctx.out(), "function <{}>", obj.get_name());
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
struct std::formatter<l3::vm::RefValue>
    : utils::static_formatter<l3::vm::RefValue> {
  static constexpr auto format(const auto &value, std::format_context &ctx) {
    return std::format_to(ctx.out(), "{}", *value);
  }
};

template <>
struct std::formatter<l3::vm::Variable>
    : utils::static_formatter<l3::vm::Variable> {
  static constexpr auto format(const auto &value, std::format_context &ctx) {
    return std::format_to(ctx.out(), "{} {}", value.get_mutability(), *value);
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
        [&ctx](const l3::vm::Value::function_type &function) {
          return std::format_to(ctx.out(), "{}", *function);
        },
        [&ctx](const l3::vm::Value::vector_type &vector) {
          return std::format_to(ctx.out(), "{}", vector);
        }
    );
  }
};

template <>
struct std::formatter<l3::vm::ValuePrettyPrinter>
    : utils::static_formatter<l3::vm::ValuePrettyPrinter> {
  static constexpr auto format(const auto &value_pp, std::format_context &ctx) {
    return value_pp.value.visit(
        [&ctx](const l3::vm::Nil &value) {
          return std::format_to(ctx.out(), "{}", value);
        },
        [&ctx](const l3::vm::Primitive &primitive) {
          return std::format_to(
              ctx.out(), "{}", l3::vm::PrimitivePrettyPrinter{primitive}
          );
        },
        [&ctx](const l3::vm::Value::function_type &function) {
          return std::format_to(ctx.out(), "{}", *function);
        },
        [&ctx](const l3::vm::Value::vector_type &vector) {
          return std::format_to(ctx.out(), "{}", vector);
        }
    );
  }
};

template <>
struct std::formatter<l3::vm::GCValue>
    : utils::static_formatter<l3::vm::GCValue> {
  static constexpr auto format(const auto &value, std::format_context &ctx) {
    if (value.is_marked()) {
      return std::format_to(ctx.out(), "{}*", value.get_value());
    }
    return std::format_to(ctx.out(), "{}", value.get_value());
  }
};
