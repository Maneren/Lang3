#pragma once

#include "utils/accessor.h"
#include <functional>

namespace l3::vm {

class GCValue;
class Value;

class RefValue {
  std::reference_wrapper<GCValue> gc_value;

public:
  explicit RefValue(GCValue &gc_value);

  [[nodiscard]] const Value &get() const;
  [[nodiscard]] Value &get();

  DEREF(get)
  DEFINE_ACCESSOR(gc, GCValue, gc_value.get());
};

} // namespace l3::vm
