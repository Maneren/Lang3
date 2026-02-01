module;

#include <compare>
#include <concepts>
#include <format>
#include <memory>
#include <optional>
#include <source_location>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

export module utils:format;

export namespace utils {

template <typename T>
concept formattable = std::semiregular<std::formatter<T>>;

template <typename... Args>
concept all_formattable = (formattable<Args> && ...);

// base formatter template that handles the parse boilerplate
template <typename t> struct static_formatter {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }
};

} // namespace utils

export {

  template <typename T, typename A>
    requires(utils::formattable<T>)
  struct std::formatter<std::vector<T, A>>
      : utils::static_formatter<std::vector<T, A>> {
    static auto format(const auto &obj, std::format_context &ctx) {
      std::format_to(ctx.out(), "[");
      for (const auto &item : obj) {
        if (&item != &obj.front()) {
          std::format_to(ctx.out(), ", ");
        }
        std::format_to(ctx.out(), "{}", item);
      }
      return std::format_to(ctx.out(), "]");
    }
  };

  template <typename T, typename H, typename P, typename A>
    requires(utils::formattable<T>)
  struct std::formatter<std::unordered_set<T, H, P, A>>
      : utils::static_formatter<std::unordered_set<T, H, P, A>> {
    static auto format(const auto &obj, std::format_context &ctx) {
      std::format_to(ctx.out(), "{{");
      bool first = true;
      for (const auto &item : obj) {
        if (!first) {
          std::format_to(ctx.out(), ", ");
        }
        std::format_to(ctx.out(), "{}", item);
        first = false;
      }
      return std::format_to(ctx.out(), "}}");
    }
  };

  template <typename T, typename U, typename H, typename P, typename A>
    requires(utils::all_formattable<T, U>)
  struct std::formatter<std::unordered_map<T, U, H, P, A>>
      : utils::static_formatter<std::unordered_map<T, U, H, P, A>> {
    static auto format(const auto &obj, std::format_context &ctx) {
      std::format_to(ctx.out(), "{{");
      bool first = true;
      for (const auto &[key, value] : obj) {
        if (!first) {
          std::format_to(ctx.out(), ", ");
        }
        std::format_to(ctx.out(), "{}: {}", key, value);
        first = false;
      }
      return std::format_to(ctx.out(), "}}");
    }
  };

  template <typename T>
    requires(utils::formattable<T>)
  struct std::formatter<std::optional<T>>
      : utils::static_formatter<std::optional<T>> {
    static auto format(const auto &obj, std::format_context &ctx) {
      if (obj.has_value()) {
        return std::format_to(ctx.out(), "{}", obj.value());
      }
      return std::format_to(ctx.out(), "nullopt");
    }
  };

  template <typename T, typename U>
    requires(utils::formattable<T> && utils::formattable<U>)
  struct std::formatter<std::pair<T, U>>
      : utils::static_formatter<std::pair<T, U>> {
    static auto format(const auto &obj, std::format_context &ctx) {
      return std::format_to(ctx.out(), "({}, {})", obj.first, obj.second);
    }
  };

  template <typename... Ts>
    requires(utils::formattable<Ts> && ...)
  struct std::formatter<std::tuple<Ts...>>
      : utils::static_formatter<std::tuple<Ts...>> {
    static auto format(const auto &obj, std::format_context &ctx) {
      auto out = std::format_to(ctx.out(), "(");
      std::apply(
          [&](const auto &...args) {
            bool first = true;
            ((out = std::format_to(out, "{}{}", first ? "" : ", ", args),
              first = false),
             ...);
          },
          obj
      );
      return std::format_to(out, ")");
    }
  };

  template <typename T>
    requires(utils::formattable<T>)
  struct std::formatter<std::unique_ptr<T>>
      : utils::static_formatter<std::optional<T>> {
    static auto format(const auto &obj, std::format_context &ctx) {
      return std::format_to(ctx.out(), "{}", *obj);
    }
  };

  template <typename T>
    requires(utils::formattable<T>)
  struct std::formatter<std::shared_ptr<T>>
      : utils::static_formatter<std::optional<T>> {
    static auto format(const auto &obj, std::format_context &ctx) {
      return std::format_to(ctx.out(), "{}", *obj);
    }
  };

  template <>
  struct std::formatter<std::source_location>
      : utils::static_formatter<std::source_location> {
    static auto format(const auto &obj, std::format_context &ctx) {
      return std::format_to(
          ctx.out(),
          "{}({},{}) `{}`",
          obj.file_name(),
          obj.line(),
          obj.column(),
          obj.function_name()
      );
    }
  };

  template <typename T>
    requires(utils::formattable<T>)
  struct std::formatter<std::reference_wrapper<T>>
      : utils::static_formatter<std::reference_wrapper<T>> {
    static auto format(const auto &obj, std::format_context &ctx) {
      return std::format_to(ctx.out(), "{}", obj.get());
    }
  };

  template <>
  struct std::formatter<std::strong_ordering>
      : utils::static_formatter<std::strong_ordering> {
    static auto format(const auto &obj, std::format_context &ctx) {
      switch (obj) {
      case std::strong_ordering::less:
        return std::format_to(ctx.out(), "less");
      case std::strong_ordering::equal:
        return std::format_to(ctx.out(), "equal");
      case std::strong_ordering::greater:
        return std::format_to(ctx.out(), "greater");
      }
    }
  };

  template <>
  struct std::formatter<std::weak_ordering>
      : utils::static_formatter<std::weak_ordering> {
    static auto format(const auto &obj, std::format_context &ctx) {
      switch (obj) {
      case std::weak_ordering::less:
        return std::format_to(ctx.out(), "less");
      case std::weak_ordering::equivalent:
        return std::format_to(ctx.out(), "equivalent");
      case std::weak_ordering::greater:
        return std::format_to(ctx.out(), "greater");
      }
    }
  };

  template <>
  struct std::formatter<std::partial_ordering>
      : utils::static_formatter<std::partial_ordering> {
    static auto format(const auto &obj, std::format_context &ctx) {
      if (obj == std::partial_ordering::equivalent) {
        return std::format_to(ctx.out(), "equivalent");
      } else if (obj == std::partial_ordering::greater) {
        return std::format_to(ctx.out(), "greater");
      } else if (obj == std::partial_ordering::less) {
        return std::format_to(ctx.out(), "less");
      } else {
        return std::format_to(ctx.out(), "unordered");
      }
    }
  };
}
