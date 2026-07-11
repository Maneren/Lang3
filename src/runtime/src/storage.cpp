module l3.runtime;

import utils;
import :gc_value;
import :value;
import :formatting;
import :chunk_list;

namespace l3::runtime {

Heap::Heap(bool debug) : debug{debug} {}
Heap::Heap(Heap &&) noexcept = default;
Heap &Heap::operator=(Heap &&) noexcept = default;
Heap::~Heap() = default;

HeapCell &Heap::emplace(Value &&value) {
  size++;
  added_since_last_sweep++;
  return backing_store.emplace_front(std::move(value));
}

std::size_t Heap::sweep() {
  debug_print("[GC] Sweeping");
  sweep_count++;
  auto erased = sweep_marked_forward_list(backing_store);
  size -= erased;
  added_since_last_sweep = 0;
  next_gc_threshold = std::max(size * 2, std::size_t{1024});
  return erased;
}

} // namespace l3::runtime
