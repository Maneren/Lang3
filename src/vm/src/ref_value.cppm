export module l3.vm:ref_value;

import std;

export namespace l3::vm {

class GCValue;
class Value;

class Ref {
  std::reference_wrapper<GCValue> gc_value;

public:
  explicit Ref(GCValue &gc_value);

  [[nodiscard]] const Value &get() const;
  [[nodiscard]] Value &get();

  DEREF(get)
  DEFINE_ACCESSOR(gc, GCValue, gc_value.get());
};

} // namespace l3::vm
