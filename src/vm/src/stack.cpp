module;

#include <vector>

module l3.vm;

import :ref_value;
import :gc_value;

namespace l3::vm {

Stack::Stack(bool debug) : debug(debug) {}

Stack::FrameGuard Stack::with_frame() { return FrameGuard(*this); }
Stack::FrameGuard::FrameGuard(Stack &stack) : stack{stack} {
  stack.frames.emplace_back();
  frame_index = stack.frames.size();
  stack.debug_print("Pushed stack frame {}", frame_index);
}
RefValue Stack::push_value(RefValue value) {
  frames.back().push_back(value);
  return value;
}
void Stack::mark_gc() {
  for (auto &frame : frames) {
    for (auto &value : frame) {
      value.get_gc_mut().mark();
    }
  }
}

Stack::FrameGuard::~FrameGuard() { // NOLINT(bugprone-exception-escape)
  stack.debug_print("Popping stack frame {}", frame_index);
  stack.frames.pop_back();
}
} // namespace l3::vm
