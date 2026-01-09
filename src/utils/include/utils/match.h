#pragma once

#include <tuple>
#include <utility>
#include <variant>

namespace match {

template <class... Ts> struct Overloaded : Ts... {
  using Ts::operator()...;
};

// Deduction guide for C++17
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

/// Pattern matching on a single std::variant.
/// Combines multiple lambdas into a single callable object and applies them to
/// the given variant.
/// @tparam Variant The type of the variant.
/// @tparam Visitors The types of the visitor lambdas.
/// @param variant The variant to match.
/// @param visitors The lambdas to apply.
/// @return The result of applying the appropriate lambda to the variant.
template <typename Variant, typename... Visitors>
constexpr decltype(auto) match(Variant &&variant, Visitors &&...visitors) {
  static_assert(
      sizeof...(Visitors) > 0, "At least one visitor must be provided"
  );
  return std::visit(
      Overloaded{std::forward<Visitors>(visitors)...},
      std::forward<Variant>(variant)
  );
}

/// Pattern matching on multiple std::variants (Cartesian product of
/// alternatives). Combines multiple lambdas into a single callable object and
/// applies them to all combinations of variant alternatives.
/// @tparam Variants The types of the variants.
/// @tparam Visitors The types of the visitor lambdas.
/// @param variants Tuple of variants to match.
/// @param visitors The lambdas to apply.
/// @return The result of applying the appropriate lambda to the variant
/// combination.
template <typename... Variants, typename... Visitors>
constexpr decltype(auto)
match(std::tuple<Variants...> &&variants, Visitors &&...visitors) {
  static_assert(
      sizeof...(Visitors) > 0, "At least one visitor must be provided"
  );
  static_assert(
      sizeof...(Variants) > 0, "At least one variant must be provided"
  );
  return std::apply(
      [&](auto &&...args) {
        return std::visit(
            Overloaded{std::forward<Visitors>(visitors)...},
            std::forward<decltype(args)>(args)...
        );
      },
      std::move(variants)
  );
}

} // namespace match
