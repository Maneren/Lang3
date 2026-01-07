#pragma once

#include "format.h"
#include "match.h"
#include <format>
#include <functional>
#include <memory>
#include <stdexcept>
#include <variant>

namespace utils {

template <typename T>
  requires(!std::is_reference_v<T>)
class Cow {
public:
  Cow(const Cow &) = default;
  Cow(Cow &&) = default;
  Cow &operator=(const Cow &) = default;
  Cow &operator=(Cow &&) = default;

  ~Cow() = default;

  explicit Cow(const T &t) : data(std::cref(t)) {}
  explicit Cow(T &t) : data(std::ref(t)) {}
  explicit Cow(T &&t) : data(std::move(t)) {}

  T &as_ref() {
    return match::match(
               data,
               [](T &t) { return std::ref(t); },
               [](std::reference_wrapper<T> &t) { return t; },
               [](std::unique_ptr<T> &t) { return std::ref(*t); }
    ).get();
  }
  const T &as_ref(this const Cow<T> &self) {
    return match::match(
               self.data,
               [](const T &t) { return std::cref(t); },
               [](const std::reference_wrapper<T> t) { return std::cref(t); },
               [](const std::unique_ptr<T> &t) { return std::cref(*t); }
    ).get();
  }

  T into_value(this Cow<T> &&self) {
    return match::match(
        std::move(self).data,
        [](T &&t) -> T { return std::move(t); },
        [](std::reference_wrapper<T>) -> T {
          throw std::runtime_error("cannot move reference wrapper");
        },
        [](std::unique_ptr<T> &&t) -> T { return *std::move(t).get(); }
    );
  }

  template <typename>
    requires std::is_copy_constructible_v<T>
  T to_value() const {
    return match::match(
        data,
        [](const T &t) -> T { return t; },
        [](const std::reference_wrapper<T> &t) -> T { return t.get(); },
        [](const std::unique_ptr<T> &t) -> T { return *t; }
    );
  }

  auto &operator*() { return as_ref(); }
  const auto &operator*() const { return as_ref(); }
  auto *operator->() { return &as_ref(); }
  const auto *operator->() const { return &as_ref(); }

private:
  std::variant<T, std::reference_wrapper<T>, std::unique_ptr<T>> data;
};

} // namespace utils

template <typename T>
  requires utils::formattable<T>
struct std::formatter<utils::Cow<T>> : utils::static_formatter<utils::Cow<T>> {
  constexpr static auto format(const auto &t, std::format_context &ctx) {
    return std::format_to(ctx.out(), "{}", t.as_ref());
  }
};
