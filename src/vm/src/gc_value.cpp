module;

#include <utility>

module l3.vm;

import :gc_value;

namespace l3::vm {

GCValue::GCValue(Value &&value) : value{std::move(value)} {}
GCValue::GCValue(GCValue &&other) noexcept
    : marked{other.marked}, value{std::move(other.value)} {
  other.marked = false;
}
GCValue &GCValue::operator=(GCValue &&other) noexcept {
  marked = other.marked;
  value = std::move(other.value);
  other.marked = false;
  return *this;
}

} // namespace l3::vm
