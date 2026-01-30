#pragma once

#include "vm/storage.hpp"
#include <vector>

namespace l3::vm {

class Stack {
  bool debug;
  std::vector<std::vector<RefValue>> frames;

  class FrameGuard {
    Stack &stack;

  public:
    explicit FrameGuard(Stack &stack) : stack{stack} {
      stack.debug_print("Pushing stack frame");
      stack.frames.emplace_back();
    }
    ~FrameGuard() {
      stack.debug_print("Popping stack frame");
      stack.frames.pop_back();
    }
  };

public:
  Stack(bool debug = false) : debug(debug) {}

  FrameGuard with_frame();
  RefValue push_value(RefValue value);

  void mark_gc();

  DEFINE_ACCESSOR_X(frames);
  DEFINE_VALUE_ACCESSOR_X(debug);

  template <typename... Ts>
  void debug_print(std::format_string<Ts...> message, Ts &&...args) const {
    if (debug) {
      std::println(std::cerr, message, std::forward<Ts>(args)...);
    }
  }
};

} // namespace l3::vm
