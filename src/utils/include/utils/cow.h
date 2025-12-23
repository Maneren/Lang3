#pragma once

#include "match.h"
#include <memory>
#include <variant>

namespace utils {

template <typename T>
  requires(!std::is_reference_v<T>)
class Cow {
public:
  explicit Cow(const T &t) : data(std::cref(t)) {}
  explicit Cow(T &t) : data(std::ref(t)) {}
  explicit Cow(T &&t) : data(std::move(t)) {}

  T *as_ref() {
    return match::match(
        data,
        [](T &t) -> T * { return &t; },
        [](std::reference_wrapper<T> &t) -> T * { return &t.get(); },
        [](std::unique_ptr<T> &t) -> T * { return &*t; }

    );
  }
  const T *as_ref() const {
    return match::match(
        data,
        [](const T &t) -> const T * { return &t; },
        [](const std::reference_wrapper<T> &t) -> const T * {
          return &t.get();
        },
        [](const std::unique_ptr<T> &t) -> const T * { return &*t; }
    );
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
    return data.visit(
        match::Overloaded{
            [](T &t) { return t; },
            [](const std::reference_wrapper<T> &t) { return t.get(); },
            [](std::unique_ptr<T> &t) { return *t; }
        }
    );
  }

private:
  std::variant<T, std::reference_wrapper<T>, std::unique_ptr<T>> data;
};

} // namespace utils
