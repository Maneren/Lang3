#pragma once

#include "format.h"
#include "utils/match.h"
#include <format>
#include <memory>
#include <utility>
#include <variant>

namespace std {
template <typename T> class reference_wrapper;
class runtime_error;
} // namespace std

namespace utils {

template <typename T>
  requires(!std::is_reference_v<T>)
class Cow {
  using ref = std::reference_wrapper<T>;
  using cref = std::reference_wrapper<const T>;
  using value = std::unique_ptr<T>;

public:
  Cow(const Cow &) = default;
  Cow(Cow &&) = default;
  Cow &operator=(const Cow &) = default;
  Cow &operator=(Cow &&) = default;

  ~Cow() = default;

  explicit Cow(const T &t) : data(std::cref(t)) {}
  explicit Cow(T &t) : data(std::ref(t)) {}
  explicit Cow(T &&t) : data(std::make_unique<T>(std::move(t))) {}

  T &as_ref() {
    return match::match(
               data,
               [](ref &t) -> ref { return t; },
               [](cref & /*t*/) -> ref { std::unreachable(); },
               [](value &t) -> ref { return std::ref(*t); }
    ).get();
  }

  const T &as_cref() const {
    return match::match(
               data,
               [](const ref &t) -> cref { return t; },
               [](const cref &t) -> cref { return t; },
               [](const value &t) -> cref { return std::cref(*t); }
    ).get();
  }

  T into_value() {
    return match::match(
        data,
        [](ref & /*t*/) -> T { throw_move_error(); },
        [](cref & /*t*/) -> T { throw_move_error(); },
        [](value &t) -> T { return std::move(*t); }
    );
  }

  template <typename U = T>
    requires std::is_copy_constructible_v<U>
  T to_value() const {
    return match::match(
        data,
        [](const ref &t) -> T { return t.get(); },
        [](const cref &t) -> T { return t.get(); },
        [](const value &t) -> T { return std::move(*t); }
    );
  }

  T &operator*() { return as_ref(); }
  const T &operator*() const { return as_cref(); }
  T *operator->() { return &as_ref(); }
  const T *operator->() const { return &as_cref(); }

private:
  std::variant<
      std::reference_wrapper<T>,
      std::reference_wrapper<const T>,
      std::unique_ptr<T>>
      data;

  [[noreturn]] static void throw_move_error();
};

template <typename T>
  requires(!std::is_reference_v<T>)
[[noreturn]] void Cow<T>::throw_move_error() {
  throw std::runtime_error("cannot move reference wrapper");
}

} // namespace utils

template <typename T>
  requires utils::formattable<T>
struct std::formatter<utils::Cow<T>> : utils::static_formatter<utils::Cow<T>> {
  constexpr static auto format(const auto &t, std::format_context &ctx) {
    return std::format_to(ctx.out(), "{}", t.as_cref());
  }
};
