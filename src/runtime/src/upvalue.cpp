module l3.runtime;

import :upvalue;

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
  std::size_t erased = 0;

  while (!backing_store.empty() && !backing_store.front().is_marked()) {
    backing_store.pop_front();
    ++erased;
  }

  if (backing_store.empty()) {
    return erased;
  }

  auto iter = backing_store.begin();
  iter->unmark();

  while (std::next(iter) != backing_store.end()) {
    auto next = std::next(iter);
    if (!next->is_marked()) {
      backing_store.erase_after(iter);
      ++erased;
    } else {
      next->unmark();
      ++iter;
    }
  }

  return erased;
}

} // namespace l3::runtime
