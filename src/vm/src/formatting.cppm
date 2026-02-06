export module l3.vm:formatting;

import utils;
import :value;
import :storage;
import :scope;

export namespace l3::vm {

struct PrimitivePrettyPrinter {
  const Primitive
      &primitive; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
};

struct ValuePrettyPrinter {
  const Value
      &value; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
};

} // namespace l3::vm

export {
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
      return primitive.visit(
          [&ctx](const bool &value) {
            return std::format_to(ctx.out(), "{}", value);
          },
          [&ctx](const std::int64_t &value) {
            return std::format_to(ctx.out(), "{}", value);
          },
          [&ctx](const double &value) {
            return std::format_to(ctx.out(), "{}", value);
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
          }
      );
    }
  };

  template <>
  struct std::formatter<l3::vm::L3Function>
      : utils::static_formatter<l3::vm::L3Function> {
    static constexpr auto format(const auto &obj, std::format_context &ctx) {
      if (obj.get_curried()) {
        return std::format_to(
            ctx.out(), "function <{}>{}", obj.get_name(), *obj.get_curried()
        );
      }
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
      return value.visit([&ctx](const auto &value) {
        return std::format_to(ctx.out(), "{}", value);
      });
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
  struct std::formatter<l3::vm::Scope>
      : utils::static_formatter<l3::vm::Scope> {
    static constexpr auto format(const auto &value, std::format_context &ctx) {
      return std::format_to(ctx.out(), "{}", value.get_variables());
    }
  };

  template <>
  struct std::formatter<l3::vm::Value>
      : utils::static_formatter<l3::vm::Value> {
    static constexpr auto format(const auto &value, std::format_context &ctx) {
      return value.visit(
          [&ctx](const l3::vm::Function &function) {
            return std::format_to(ctx.out(), "{}", function);
          },
          [&ctx](const l3::vm::Value::string_type &value) {
            return std::format_to(ctx.out(), R"("{}")", value);
          },
          [&ctx](const auto &value) {
            return std::format_to(ctx.out(), "{}", value);
          }
      );
    }
  };

  template <>
  struct std::formatter<l3::vm::ValuePrettyPrinter>
      : utils::static_formatter<l3::vm::ValuePrettyPrinter> {
    static constexpr auto
    format(const auto &value_pp, std::format_context &ctx) {
      return value_pp.value.visit(
          [&ctx](const l3::vm::Primitive &primitive) {
            return std::format_to(
                ctx.out(), "{}", l3::vm::PrimitivePrettyPrinter{primitive}
            );
          },
          [&ctx](const l3::vm::Value::string_type &value) {
            return std::format_to(ctx.out(), "{}", value);
          },
          [&ctx](const auto &value) {
            return std::format_to(ctx.out(), "{}", value);
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
}
