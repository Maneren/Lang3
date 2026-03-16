module l3.runtime;

import :gc_value;
import :value;

namespace l3::runtime {

Ref::Ref(GCValue &gc_value) : gc_value{gc_value} {}
const Value &Ref::get() const { return get_gc().get_value(); }
Value &Ref::get() { return get_gc_mut().get_value_mut(); }

} // namespace l3::runtime
