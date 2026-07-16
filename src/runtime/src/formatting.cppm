export module l3.runtime:formatting;

import utils;

import :heap;
import :heap_cell;
import :heap_data;
import :stack_value;

export namespace l3::runtime {} // namespace l3::runtime

export {
  template <>
  struct std::formatter<l3::runtime::Nil>
      : utils::static_formatter<l3::runtime::Nil> {
    static constexpr auto
    format(const auto & /*unused*/, std::format_context &ctx) {
      return std::format_to(ctx.out(), "nil");
    }
  };

  template <>
  struct std::formatter<l3::runtime::Primitive>
      : utils::static_formatter<l3::runtime::Primitive> {
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
  struct std::formatter<l3::runtime::BuiltinFunction>
      : utils::static_formatter<l3::runtime::BuiltinFunction> {
    static constexpr auto format(const auto &obj, std::format_context &ctx) {
      return std::format_to(ctx.out(), "function <{}>", obj.get_name());
    }
  };

  template <>
  struct std::formatter<l3::runtime::BytecodeFunction>
      : utils::static_formatter<l3::runtime::BytecodeFunction> {
    static constexpr auto format(const auto &obj, std::format_context &ctx) {
      return std::format_to(ctx.out(), "function <{}>", obj.name);
    }
  };

  template <>
  struct std::formatter<l3::runtime::Function>
      : utils::static_formatter<l3::runtime::Function> {
    static constexpr auto format(const auto &value, std::format_context &ctx) {
      return value.visit([&ctx](const auto &value) {
        return std::format_to(ctx.out(), "{}", value);
      });
    }
  };

  template <>
  struct std::formatter<l3::runtime::StackValue>
      : utils::static_formatter<l3::runtime::StackValue> {
    static constexpr auto format(const auto &sv, std::format_context &ctx) {
      return sv.visit(
          [&ctx](const l3::runtime::Primitive &primitive) {
            return std::format_to(ctx.out(), "{}", primitive);
          },
          [&ctx](const l3::runtime::Nil &) {
            return std::format_to(ctx.out(), "nil");
          },
          [&ctx](const l3::runtime::HeapCell *gcv) -> decltype(auto) {
            return gcv->get_value().visit(
                [&ctx](const l3::runtime::HeapData::function_type &f) {
                  return std::format_to(ctx.out(), "{}", *f);
                },
                [&ctx](const std::vector<l3::runtime::StackValue> &vec) {
                  auto out = std::format_to(ctx.out(), "[");
                  for (std::size_t i = 0; i < vec.size(); ++i) {
                    if (i > 0)
                      out = std::format_to(out, ", ");
                    out = std::format_to(out, "{}", vec[i]);
                  }
                  return std::format_to(out, "]");
                },
                [&ctx](const std::string &s) {
                  return std::format_to(ctx.out(), "{}", s);
                },
                [&ctx](const l3::runtime::Primitive &p) {
                  return std::format_to(ctx.out(), "{}", p);
                },
                [&ctx](const l3::runtime::Nil &) {
                  return std::format_to(ctx.out(), "nil");
                }
            );
          }
      );
    }
  };

  template <>
  struct std::formatter<l3::runtime::HeapData>
      : utils::static_formatter<l3::runtime::HeapData> {
    static constexpr auto format(const auto &value, std::format_context &ctx) {
      return value.visit(
          [&ctx](const l3::runtime::HeapData::function_type &function) {
            return std::format_to(ctx.out(), "{}", *function);
          },
          [&ctx](const l3::runtime::HeapData::string_type &value) {
            return std::format_to(ctx.out(), R"("{}")", value);
          },
          [&ctx](const l3::runtime::Primitive &primitive) {
            return std::format_to(ctx.out(), "{}", primitive);
          },
          [&ctx](const l3::runtime::Nil &) {
            return std::format_to(ctx.out(), "nil");
          },
          [&ctx](const l3::runtime::HeapData::vector_type &vec) {
            auto out = std::format_to(ctx.out(), "[");
            for (std::size_t i = 0; i < vec.size(); ++i) {
              if (i > 0) {
                out = std::format_to(out, ", ");
              }
              out = std::format_to(out, "{}", vec[i]);
            }
            return std::format_to(out, "]");
          }
      );
    }
  };

  template <>
  struct std::formatter<l3::runtime::HeapCell>
      : utils::static_formatter<l3::runtime::HeapCell> {
    static constexpr auto format(const auto &value, std::format_context &ctx) {
      if (value.is_marked()) {
        return std::format_to(ctx.out(), "{}*", value.get_value());
      }
      return std::format_to(ctx.out(), "{}", value.get_value());
    }
  };
}
