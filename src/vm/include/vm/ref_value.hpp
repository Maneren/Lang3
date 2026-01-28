#pragma once

#include "utils/accessor.h"
#include <functional>

namespace l3::vm {

struct GCValue;
class Value;

struct RefValue {
  explicit RefValue(GCValue &gc_value);

  [[nodiscard]] const Value &get() const;
  [[nodiscard]] Value &get();

  DEFINE_ACCESSOR(gc, GCValue, gc_value);

  [[nodiscard]] Value &operator*() { return get(); }
  [[nodiscard]] const Value &operator*() const { return get(); }

  Value *operator->() { return &get(); }
  const Value *operator->() const { return &get(); }

private:
  std::reference_wrapper<GCValue> gc_value;
};

} // namespace l3::vm
