#pragma once

#define VISIT(name)                                                            \
  auto visit(this auto &&self, auto &&...visitor) {                            \
    return match::match(                                                       \
        self.name, std::forward<decltype(visitor)>(visitor)...                 \
    );                                                                         \
  }
