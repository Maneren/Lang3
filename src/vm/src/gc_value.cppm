module;

#include <utility>
#include <utils/accessor.h>

export module l3.vm:gc_value;

import :value;

export namespace l3::vm {

class GCValue {
  bool marked = false;
  Value value;

public:
  GCValue(Value &&value) : value{std::move(value)} {};
  GCValue(const GCValue &) = delete;
  GCValue(GCValue &&other) noexcept
      : marked{other.marked}, value{std::move(other.value)} {
    other.marked = false;
  }
  GCValue &operator=(const GCValue &) = delete;
  GCValue &operator=(GCValue &&other) noexcept {
    marked = other.marked;
    value = std::move(other.value);
    other.marked = false;
    return *this;
  }
  ~GCValue() = default;

  void mark();
  void unmark() { marked = false; }

  [[nodiscard]] bool is_marked() const { return marked; }

  DEFINE_ACCESSOR_X(value);
};

} // namespace l3::vm
