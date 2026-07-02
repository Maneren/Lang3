export module utils:match;

import std;

struct DeduceReturn {};

template <typename T> struct is_optional : std::false_type {};
template <typename T> struct is_optional<std::optional<T>> : std::true_type {};
template <typename T>
inline constexpr bool is_optional_v = is_optional<T>::value;

template <typename R, typename Visitor, typename... Variants>
constexpr decltype(auto)
visit_dispatch(Visitor &&visitor, Variants &&...variants) {
  if constexpr (is_optional_v<R>) {
    return std::visit(
        [&](auto &&...args) -> R {
          using ReturnType = std::invoke_result_t<Visitor, decltype(args)...>;
          using DecayReturnType = std::decay_t<ReturnType>;

          if constexpr (std::is_void_v<ReturnType>) {
            std::invoke(
                std::forward<Visitor>(visitor),
                std::forward<decltype(args)>(args)...
            );
            return std::nullopt;
          }
          // If the lambda explicitly returns std::nullopt or another
          // std::optional, do not wrap it in std::in_place
          else if constexpr (
              std::is_same_v<DecayReturnType, std::nullopt_t> ||
              is_optional_v<DecayReturnType>
          ) {
            return std::invoke(
                std::forward<Visitor>(visitor),
                std::forward<decltype(args)>(args)...
            );
          } else {
            return R(
                std::in_place,
                std::invoke(
                    std::forward<Visitor>(visitor),
                    std::forward<decltype(args)>(args)...
                )
            );
          }
        },
        std::forward<Variants>(variants)...
    );
  } else {
    return std::visit(
        std::forward<Visitor>(visitor), std::forward<Variants>(variants)...
    );
  }
}

export namespace match {

template <class... Ts> struct Overloaded : Ts... {
  using Ts::operator()...;
};

/// Pattern matching on multiple std::variants (Cartesian product of
/// alternatives). Same as match(Variant &&variant, Visitors &&...visitors)
/// except that the lambdas take multiple arguments.
/// It is required that exactly one lambda matches each variant alternative
/// combination (there may be more than one alternative combination per lambda).
/// If R is specified as a std::optional, raw return values are automatically
/// wrapped, void branches return std::nullopt, and explicit std::nullopt
/// returns are permitted.
/// @tparam R The expected return type for all visitors (defaults to automatic
/// deduction).
/// @tparam Variants The types of the variants.
/// @tparam Visitors The types of the visitor lambdas.
/// @param variants Tuple of variants to match.
/// @param visitors The lambdas to apply.
/// @return The result of applying the appropriate lambda to the variant
/// combination.
template <typename R = DeduceReturn, typename... Variants, typename... Visitors>
constexpr decltype(auto)
match(std::tuple<Variants...> &&variants, Visitors &&...visitors) {
  static_assert(
      sizeof...(Visitors) > 0, "At least one visitor must be provided"
  );
  static_assert(
      sizeof...(Variants) > 0, "At least one variant must be provided"
  );

  return std::apply(
      [&](auto &&...args) -> decltype(auto) {
        return visit_dispatch<R>(
            Overloaded{std::forward<Visitors>(visitors)...},
            std::forward<decltype(args)>(args)...
        );
      },
      std::move(variants)
  );
}

/// Pattern matching on a std::variant.
/// Takes a variant and a set of lambdas and applies the matching lambda to the
/// variant according to the contained alternative.
/// It is required that exactly one lambda matches each variant alternative
/// (there may be more than one alternative per lambda).
/// The matching follows the standard overload resolution rules.
/// If R is specified as a std::optional, raw return values are automatically
/// wrapped, void branches return std::nullopt, and explicit std::nullopt
/// returns are permitted.
/// @tparam R The expected return type for all visitors (defaults to automatic
/// deduction).
/// @tparam Variant The type of the variant.
/// @tparam Visitors The types of the visitor lambdas.
/// @param variant The variant to match.
/// @param visitors The lambdas to apply.
/// @return The result of applying the appropriate lambda to the variant.
template <typename R = DeduceReturn, typename Variant, typename... Visitors>
constexpr decltype(auto) match(Variant &&variant, Visitors &&...visitors) {
  return match<R>(
      std::forward_as_tuple(std::forward<Variant>(variant)),
      std::forward<Visitors>(visitors)...
  );
}

} // namespace match
