module l3.runtime;

import :gc_value;

namespace l3::runtime {

StackValue::StackValue() : inner{Nil{}} {}
StackValue::StackValue(Nil /*unused*/) : inner{Nil{}} {}
StackValue::StackValue(Primitive primitive) : inner{primitive} {}
StackValue::StackValue(HeapCell *gc_value) : inner{gc_value} {
  // HeapCell* variant must never be null — nil is always represented by Nil
}

bool StackValue::is_nil() const { return std::holds_alternative<Nil>(inner); }

bool StackValue::is_primitive() const {
  return std::holds_alternative<Primitive>(inner);
}

utils::optional_cref<Primitive> StackValue::as_primitive() const {
  if (const auto *p = std::get_if<Primitive>(&inner)) {
    return *p;
  }
  return std::nullopt;
}

bool StackValue::is_string() const {
  if (auto *gcv = get_heap_ptr()) {
    return gcv->get_value().is_string();
  }
  return false;
}

bool StackValue::is_vector() const {
  if (auto *gcv = get_heap_ptr()) {
    return gcv->get_value().is_vector();
  }
  return false;
}

bool StackValue::is_function() const {
  if (auto *gcv = get_heap_ptr()) {
    return gcv->get_value().is_function();
  }
  return false;
}

utils::optional_cref<std::string> StackValue::as_string() const {
  if (auto *gcv = get_heap_ptr()) {
    return gcv->get_value().as_string();
  }
  return std::nullopt;
}

utils::optional_cref<std::vector<StackValue>> StackValue::as_vector() const {
  if (auto *gcv = get_heap_ptr()) {
    return gcv->get_value().as_vector();
  }
  return std::nullopt;
}

utils::optional_ref<std::vector<StackValue>> StackValue::as_mut_vector() {
  if (auto *gcv = get_heap_ptr()) {
    return gcv->get_value().as_mut_vector();
  }
  return std::nullopt;
}

HeapData StackValue::slice(Slice slice) const {
  if (auto *gcv = get_heap_ptr()) {
    return gcv->get_value().slice(slice);
  }
  throw RuntimeError("slice() requires a string or vector");
}

} // namespace l3::runtime
