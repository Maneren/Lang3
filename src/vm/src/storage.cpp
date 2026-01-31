module;

#include <iterator>
#include <utility>

module l3.vm;

import utils;
import :gc_value;
import :value;

namespace l3::vm {

GCStorage::GCStorage(bool debug) : debug{debug} {}
GCStorage::GCStorage(GCStorage &&) noexcept = default;
GCStorage &GCStorage::operator=(GCStorage &&) noexcept = default;
GCStorage::~GCStorage() = default;

GCValue &GCStorage::emplace(Value &&value) {
  size++;
  added_since_last_sweep++;
  return backing_store.emplace_front(std::move(value));
}

size_t GCStorage::sweep() {
  debug_print("[GC] Sweeping");
  sweep_count++;
  added_since_last_sweep = 0;
  size_t erased = 0;
  // Remove all unmarked items from the front to ensure we start from a marked
  // value
  while (!backing_store.empty() && !backing_store.front().is_marked()) {
    backing_store.pop_front();
    ++erased;
    --size;
  }

  if (backing_store.empty()) {
    return erased;
  }

  // Erase the unmarked items and reset the marks
  auto iter = backing_store.begin();
  iter->unmark();

  while (std::next(iter) != backing_store.end()) {
    auto next = std::next(iter);
    if (!next->is_marked()) {
      backing_store.erase_after(iter);
      ++erased;
      --size;
    } else {
      next->unmark();
      ++iter;
    }
  }

  return erased;
}

void GCValue::mark() {
  if (marked) {
    return;
  }

  marked = true;

  get_value_mut().visit(
      [](Value::vector_ptr_type &vector) {
        for (auto &item : *vector) {
          item.get_gc_mut().mark();
        }
      },
      [](auto & /*value*/) {}
  );
}

GCValue GCStorage::NIL{Value()};
GCValue GCStorage::TRUE{Value{Primitive{true}}};
GCValue GCStorage::FALSE{Value{Primitive{false}}};

} // namespace l3::vm
