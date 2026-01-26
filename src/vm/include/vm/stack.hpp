#pragma once

#include "vm/storage.hpp"
#include <vector>

namespace l3::vm {

class Stack {
public:
  Stack(bool debug = false) : debug(debug) {}

  std::vector<RefValue> &top_frame();
  void push_frame();
  void pop_frame();
  RefValue push_value(RefValue value);

  void mark_gc();

  template <typename... Ts>
  void debug_print(std::format_string<Ts...> message, Ts &&...args) const {
    if (debug) {
      std::println(std::cerr, message, std::forward<Ts>(args)...);
    }
  }

private:
  bool debug;
  std::vector<std::vector<RefValue>> frames{{}};
};

} // namespace l3::vm
