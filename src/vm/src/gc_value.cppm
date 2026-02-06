export module l3.vm:gc_value;

import :value;
import :variable;

export namespace l3::vm {

class GCValue {
  Value value;
  bool marked = false;

public:
  GCValue(Value &&value);
  GCValue(const GCValue &) = delete;
  GCValue(GCValue &&other) noexcept;
  GCValue &operator=(const GCValue &) = delete;
  GCValue &operator=(GCValue &&other) noexcept;
  ~GCValue() = default;

  void mark();
  void unmark() { marked = false; }

  [[nodiscard]] bool is_marked() const { return marked; }

  DEFINE_ACCESSOR_X(value);
};

} // namespace l3::vm
