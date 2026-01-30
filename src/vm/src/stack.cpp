#include "vm/stack.hpp"

namespace l3::vm {

Stack::FrameGuard Stack::with_frame() { return FrameGuard(*this); }
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

} // namespace l3::vm
