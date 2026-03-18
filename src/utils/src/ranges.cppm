export module utils:ranges;

import std;

export namespace utils::ranges {

auto enumerate(auto &&rng) {
  return std::views::zip(
      std::views::iota(0UZ), std::forward<decltype(rng)>(rng)
  );
}

} // namespace utils::ranges
