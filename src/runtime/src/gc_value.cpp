module l3.runtime;

import :gc_value;
import :upvalue;

namespace l3::runtime {

GCValue::GCValue(Value &&value) : value{std::move(value)} {}
GCValue::GCValue(GCValue &&other) noexcept = default;
GCValue &GCValue::operator=(GCValue &&other) noexcept = default;

void GCValue::mark() {
  if (marked) {
    return;
  }

  marked = true;

  auto mark_sv = [](StackValue &sv) {
    if (auto *gcv = sv.get_gc_ptr()) {
      gcv->mark();
    }
  };

  value.visit(
      [&](std::vector<StackValue> &vector) {
        for (auto &item : vector) {
          mark_sv(item);
        }
      },
      [&](Function &func) {
        if (auto bc_opt = func.as_mut_bytecode_function()) {
          for (auto &ca : bc_opt->get().curried_args) {
            mark_sv(ca);
          }
          for (auto *uv : bc_opt->get().captured_upvalue_refs) {
            uv->mark();
          }
        }
      },
      [](auto &) {}
  );
}

} // namespace l3::runtime
