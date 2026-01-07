#pragma once

#include <tuple>
#include <variant>

namespace match {

namespace {

#include <type_traits>

template <typename...> struct is_all_same : std::false_type {};

template <> struct is_all_same<> : std::true_type {};

template <typename T> struct is_all_same<T> : std::true_type {};

template <typename T> struct is_all_same<T, T> : std::true_type {};

template <typename T, typename... U>
struct is_all_same<T, T, U...> : is_all_same<T, U...> {};

template <typename... Ts>
constexpr inline bool is_all_same_v = is_all_same<Ts...>::value;

} // namespace

template <class... Ts> struct Overloaded : Ts... {
  using Ts::operator()...;
};

/// A utility function for pattern matching on std::variant.
/// Combines multiple lambdas into a single callable object and applies them to
/// the given variant.
/// @tparam T The type of the variant.
/// @tparam Args The types of the lambdas.
/// @param t The variant to match.
/// @param args The lambdas to apply.
/// @return The result of applying the appropriate lambda to the variant.
template <typename... Variants, typename... Visitors>
constexpr auto
match(const std::tuple<Variants...> variants, Visitors &&...visitors) {
  static_assert(
      sizeof...(Visitors) > 0, "At least one visitor must be provided"
  );
  static_assert(
      sizeof...(Variants) > 0, "At least one variant must be provided"
  );
  return std::apply(
      [&visitors...](auto &&...args) {
        return std::visit(
            Overloaded{std::forward<Visitors>(visitors)...},
            std::forward<decltype(args)>(args)...
        );
      },
      std::move(variants)
  );
}

template <typename Variant, typename... Visitors>
constexpr auto match(const Variant &variant, Visitors &&...visitors) {
  return match(
      std::forward_as_tuple(variant), std::forward<Visitors>(visitors)...
  );
}

/// A utility function for pattern matching on std::variant.
/// Combines multiple lambdas into a single callable object and applies them to
/// the given variant.
/// @tparam T The type of the variant.
/// @tparam Args The types of the lambdas.
/// @param t The variant to match.
/// @param args The lambdas to apply.
/// @return The result of applying the appropriate lambda to the variant.
template <typename Variant, typename... Visitors>
constexpr auto match(Variant &&variants, Visitors &&...visitors) {
  static_assert(
      sizeof...(visitors) > 0, "At least one visitor must be provided"
  );
  return std::visit(
      Overloaded{std::forward<Visitors>(visitors)...},
      std::forward<Variant>(variants)
  );
}

} // namespace match
