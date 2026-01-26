#include "vm/stack.hpp"
#include "vm/format.hpp"

namespace l3::vm {

std::vector<RefValue> &Stack::top_frame() { return frames.back(); };
void Stack::push_frame() {
  debug_print("Pushing frame");
  frames.emplace_back();
}
void Stack::pop_frame() {
  debug_print("Popping frame");
  frames.pop_back();
}
RefValue Stack::push_value(RefValue value) {
  debug_print("Pushing value '{}' on stack", value);
  top_frame().push_back(value);
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
