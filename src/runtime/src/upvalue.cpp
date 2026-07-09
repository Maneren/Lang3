module l3.runtime;

import :upvalue;
import :chunk_list;
import :gc_value;

namespace l3::runtime {

GCUpvalue::GCUpvalue(StackValue &&value) : value{std::move(value)} {}
GCUpvalue::GCUpvalue(const StackValue &value) : value{value} {}

void GCUpvalue::mark() {
  if (marked) {
    return;
  }

  marked = true;

  if (auto *gcv = value.get_gc_ptr()) {
    gcv->mark();
  }
}

std::size_t GCUpvalueStorage::sweep() {
  return sweep_marked_forward_list(backing_store);
}

} // namespace l3::runtime
