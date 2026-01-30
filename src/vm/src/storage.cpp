#include <ranges>
#include <vm/format.hpp>
#include <vm/scope.hpp>
#include <vm/storage.hpp>
#include <vm/value.hpp>

namespace l3::vm {

GCValue::GCValue(Value &&value) : value{std::move(value)} {}

GCValue &GCStorage::emplace(Value &&value) {
  debug_print("[GC] Emplacing {}", value);
  return backing_store.emplace_front(std::move(value));
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

  // Erase the unmarked items and reset the marks
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
  if (marked) {
    return;
  }

  marked = true;

  if (auto vec = get_value_mut().as_mut_vector()) {
    for (auto &item : vec->get()) {
      item.get_gc_mut().mark();
    }
  } else if (auto fn = get_value_mut().as_function()) {
    fn->get()->visit(
        [](L3Function &function) {
          for (auto &scope : function.get_captures_mut()) {
            scope->mark_gc();
          }
        },
        [](BuiltinFunction & /*function*/) {}
    );
  }
}

GCValue GCStorage::NIL{Value()};
GCValue GCStorage::TRUE{Value{Primitive{true}}};
GCValue GCStorage::FALSE{Value{Primitive{false}}};

RefValue::RefValue(GCValue &gc_value) : gc_value{gc_value} {}
[[nodiscard]] const Value &RefValue::get() const {
  return get_gc().get_value();
}
[[nodiscard]] Value &RefValue::get() {
  // std::println("Mutably referencing {}", get_gc().get_value());
  return get_gc_mut().get_value_mut();
}

} // namespace l3::vm
