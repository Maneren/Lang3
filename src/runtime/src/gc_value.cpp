module l3.runtime;

import :gc_value;

namespace l3::runtime {

GCValue::GCValue(Value &&value) : value{std::move(value)} {}
GCValue::GCValue(GCValue &&other) noexcept
    : value{std::move(other.value)}, marked{other.marked} {
  other.marked = false;
}
GCValue &GCValue::operator=(GCValue &&other) noexcept {
  marked = other.marked;
  value = std::move(other.value);
  other.marked = false;
  return *this;
}

void GCValue::mark() {
  if (marked) {
    return;
  }

  marked = true;

  get_value_mut().visit(
      [](Value::vector_type &vector) {
        for (auto &item : vector) {
          item.get_gc_mut().mark();
        }
      },
      [](Value::function_type &func) {
        if (auto bc_opt = func->as_mut_bytecode_function()) {
          for (auto &uv : bc_opt->get().upvalues) {
            uv.get_gc_mut().mark();
          }
          for (auto &ca : bc_opt->get().curried_args) {
            ca.get_gc_mut().mark();
          }
        }
      },
      [](auto & /*value*/) {}
  );
}

} // namespace l3::runtime
