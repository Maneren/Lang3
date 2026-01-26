#pragma once

#include <tuple>
#include <utility>
#include <variant>

namespace match {

/// Convert a set of lambdas into a single overloaded callable object.
/// @tparam Ts The types of the lambdas.
template <class... Ts> struct Overloaded : Ts... {
  using Ts::operator()...;
};

// Deduction guide for C++17
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

/// Pattern matching on a std::variant.
/// Takes a variant and a set of lambdas and applies the matching lambda to the
/// variant according to the contained alternative.
/// It is require that exactly one lambda matches each variant alternative
/// (there may be more than one alternative per lambda) and that all lamdas have
/// the same return type. The matching follows the standard overload resolution
/// rules.
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
/// alternatives). Same as match(Variant &&variant, Visitors &&...visitors)
/// except that the lambdas take multiple arguments.
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
