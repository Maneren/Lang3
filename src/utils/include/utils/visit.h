#pragma once

#define VISIT(name)                                                            \
  auto visit(auto &&...visitor) const -> decltype(auto) {                      \
    return match::match(name, std::forward<decltype(visitor)>(visitor)...);    \
  }                                                                            \
  auto visit(auto &&...visitor) -> decltype(auto) {                            \
    return match::match(name, std::forward<decltype(visitor)>(visitor)...);    \
  }
