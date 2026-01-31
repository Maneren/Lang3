module l3.vm;

import :gc_value;
import :value;

namespace l3::vm {

RefValue::RefValue(GCValue &gc_value) : gc_value{gc_value} {}
const Value &RefValue::get() const {
  return get_gc().get_value();
}
Value &RefValue::get() { return get_gc_mut().get_value_mut(); }

} // namespace l3::vm
