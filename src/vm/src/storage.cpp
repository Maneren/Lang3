#include <print>
#include <vm/format.hpp>
#include <vm/storage.hpp>
#include <vm/value.hpp>

namespace l3::vm {

GCValue &GCStorage::emplace(Value &&value) {
  debug_print("[GC] Emplacing {}", value);
  return backing_store.emplace_front(std::make_shared<Value>(std::move(value)));
}
GCValue &GCStorage::emplace(const std::shared_ptr<Value> &value) {
  debug_print("[GC] Emplacing {}", value);
  return backing_store.emplace_front(value);
}

size_t GCStorage::sweep() {
  debug_print("[GC] Sweeping");
  size_t erased = 0;
  // Remove all unmarked items from the front to ensure we start from a marked
  // value
  while (!backing_store.empty() && !backing_store.front().is_marked()) {
    debug_print("[GC] Erasing {}", backing_store.front().get_value());
    backing_store.pop_front();
    ++erased;
  }

  if (backing_store.empty()) {
    return erased;
  }

  // Reset the marks for or erase the rest
  auto iter = backing_store.begin();
  iter->unmark();

  while (std::next(iter) != backing_store.end()) {
    auto next = std::next(iter);
    if (!next->is_marked()) {
      debug_print("[GC] Erasing {}", next->get_value());
      backing_store.erase_after(iter);
      ++erased;
    } else {
      next->unmark();
      ++iter;
    }
  }

  return erased;
}

void GCValue::mark() {
  marked = true;

  if (auto vec = get_value_mut().as_mut_vector()) {
    for (auto &item : vec->get()) {
      item.get_gc_mut().mark();
    }
  }
}

GCValue GCStorage::NIL{std::make_shared<Value>()};

} // namespace l3::vm
