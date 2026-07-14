module l3.runtime;

import :upvalue;
import :chunked_allocator;
import :heap_cell;

namespace l3::runtime {

UpvalueCell::UpvalueCell(StackValue &&value) : value{std::move(value)} {}
UpvalueCell::UpvalueCell(const StackValue &value) : value{value} {}

void UpvalueCell::mark() {
  if (marked) {
    return;
  }

  marked = true;

  if (auto *gcv = value.get_heap_ptr()) {
    gcv->mark();
  }
}

std::size_t UpvalueStorage::sweep() {
  return sweep_marked_forward_list(backing_store);
}

} // namespace l3::runtime
